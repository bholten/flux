namespace eval ::___flux::main {
    variable requests
    variable current_request

    if {![info exists requests]} {
	set requests {}
    }

    proc add-request {} {
	lappend ::___flux::main::requests $::___flux::main::current_request
    }

    proc create_request {verb url body} {
	set req_name "GET/$url"
	set ::___flux::main::current_request [dict create verb GET headers [list] data ""]

	set full_ns ::___flux::main::${req_name}
	namespace eval $full_ns {}
	namespace eval ${full_ns}::config {}
	namespace eval ::___flux::main $body

	if {[info exist ::___flux::main::current_request]} {
	    ::___flux::main::add-request
	    unset ::___flux::main::current_request
	}
    }

    proc do_request {req} {
	puts "TODO $req - do request here"
    }

    proc run {} {
	foreach {req} $requests {
	    do_request $req
	}
    }
}

proc GET {url body} {
    ::___flux::main::create_request GET $url $body
}

proc POST {url body} {
    ::___flux::main::create_request POST $url $body
}

proc PUT {url body} {
    ::___flux::main::create_request PUT $url $body
}

proc DELETE {url body} {
    ::___flux::main::create_request DELETE $url $body
}

proc PATCH {url body} {
    ::___flux::main::create_request PATCH $url $body
}

proc OPTIONS {url body} {
    ::___flux::main::create_request OPTIONS $url $body
}

proc headers {hds} {
    set headers [split [subst $hds] "\n"]
    set trim_headers [lmap header $headers {
	set h [string trim $header]
	if {![string equal $h ""]} {
	    set header $h
	} else {
	    continue
	}
    }]

    puts "Headers: $trim_headers"

    dict update ::___flux::main::current_request headers hds {
	set hds $trim_headers
    }
}

proc data {bd} {
    set bd [subst $bd]

    puts "Data: $bd"

    dict update ::___flux::main::current_request data dt {
	set dt $bd
    }
}
