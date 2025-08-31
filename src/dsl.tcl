namespace eval ::flux::assert {
    variable response

    variable failures
    set failure 0

    variable status_code
    variable content_type
    variable body
    variable headers
    variable json

    set status_code -1
    set content_type ""
    set body {}
    set headers {}
    set json {}

    proc _unescape {seg} {
        set seg [string map [list ~1 / ~0 ~] $seg]
        return $seg
    }

    proc json_at {pointer} {
        variable json

        if {$pointer eq "" || $pointer eq "/"} {
            return $json
        }

        if {![string match "/*" ${pointer}]} {
            error "json_a: pointer must start with /"
        }

        set node $json

        foreach raw [lrange [split $pointer "/"] 1 end] {
            set seg [_unescape $raw]

            if {[dict exists $node $seg]} {
                set node [dict get $node $seg]
            } elseif {[string is integer -strict $seg] && [llength $node] > $seg} {
                set node [lindex $node $seg]
            } else {
                error "json_at: path not found $pointer"
            }
        }

        return $node
    }

    proc print_response {} {
        variable response
        puts $response
    }

    proc fail {msg} {
        variable failures
        incr failures
        error "Assertion failed: $msg"
    }

    proc equal {actual expected} {
        if {$actual ne $expected} {
            fail "equal: expected <$expected> got <$actual>"
        }
    }

    proc matches {actual pattern} {
        if {![regexp -- $pattern $actual]} {
            fail "matches: <$actual> !~ $pattern"
        }
    }

    proc headers_equal {name expected} {
        variable headers
        set key [string tolower $name]

        if {![dict exists $headers $key]} {
            fail "header_equal: missing header $name"
        }

        set vals [dict get $headers $key]
        set first [lindex $vals 0]

        if {$fist ne $expected} {
            fail "header_equal: expected <$expected> got <$first> for $name"
        }
    }

    proc header_exists {name} {
        variable headers
        if {![dict exists $headers [string tolower $name]]} {
            fail "header_exists: $name not present"
        }
    }

    proc body_contains {needle} {
        variable body
        if {[string first $needle $body] < 0} {
            fail "body_contains: not found <$needle>"
        }
    }
}

namespace eval ::flux::__internal {
    variable uid 0

    proc gensym {{prefix "n"}} {
        variable uid
        incr uid
        return "${prefix}${uid}"
    }

    proc safe_name {s} {
        set s [string tolower $s]
        regsub -all {[^a-z0-9_]+} $s {_} s
        return $s
    }

    proc split_key_values {value_string} {
        set values [split ${value_string} "\n"]

        set trim_configs [lmap config $values {
            set c [string trim $config]
            if {![string equal $c ""]} {
                set config $c
            } else {
                continue
            }
    }]

        return $trim_configs
    }
}

namespace eval ::flux {
    variable assertions
    variable request
    variable steps
    variable response

    set assertions {}
    set request {}
    set steps {}
    set response ""


    proc headers {hds} {
        variable request

        set hs [split $hds "\n"]
        set hs [lmap h $hs {
            set entry [string trim $h]
            if {[string equal $entry ""]} {
                continue
            }

            set entry [string trim [subst $entry]]
            set h $entry
        }]

        set request [dict set request headers $hs]
    }

    proc options {opts} {
        variable request

        set os [list {*}[subst $opts]]
        set os [lmap o $os {
            set entry [string trim $o]
            if {[string equal $entry ""]} {
                continue
            }

            set entry [string trim $entry ":"]
            set o $entry
        }]

        set request [dict set request options $os]
    }

    proc data {d} {
        variable request
        set request [dict set request data $d]
    }

    proc asserts {body} {
        variable assertions
        set assertions $body
    }

    proc http_request {verb url body} {
        variable request
        variable steps

        set request {}

        set request_namespace [format "%s::%s" \
                                   [namespace current] \
                                   [::flux::__internal::gensym req_]]

        uplevel 1 $body

        set entry [dict create \
                       ns $request_namespace \
                       verb $verb \
                       url $url \
                       request $request]

        lappend steps $entry

        return $entry
    }


    proc GET {url body} {
        return [http_request GET $url $body]
    }

    proc POST {url body} {
        return [http_request POST $url $body]
    }

    proc PUT {url body} {
        return [http_request PUT $url $body]
    }

    proc DELETE {url body} {
        return [http_request DELETE $url $body]
    }

    proc PATCH {url body} {
        return [http_request PATCH $url $body]
    }

    proc OPTIONS {url body} {
        return [http_request OPTIONS $url $body]
    }

    proc run {} {
        variable assertions
        variable response
        variable steps

        foreach req $steps {
            set h [flux.init]

            $h configure -url [dict get $req url]
            $h configure -verb [dict get $req verb]

            if {[dict exists $req request]} {
                set r [dict get $req request]

                if {[dict exists $r options]} {
                    dict for {key value} [dict get $r options] {
                        $h configure -$key $value
                    }
                }

                if {[dict exists $r headers]} {
                    foreach {key} [dict get $r headers] {
                        $h header $key
                    }
                }

                if {[dict exists $r data]} {
                    $h body [dict get $r data]
                }
            }

            $h perform
            set response [$h result]
            puts "RESULT $response"
            $h reset

            if {$assertions ne {}} {
                namespace eval assert [list variable response $response]
                namespace eval assert [subst {
                    $assertions
                }]
            }
        }
    }
}
