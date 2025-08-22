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
    variable request
    variable steps

    set request {}
    set steps {}

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
	    $h reset
	}
    }
}
