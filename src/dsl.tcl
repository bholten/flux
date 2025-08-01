namespace eval ::flux {
    variable requests
    variable current_request

    if {![info exists requests]} {
	set requests [list]
    }

    proc add-request {} {
	lappend ::flux::requests $::flux::current_request
    }
}

proc GET {url body} {
    set req_name "GET $url"
    set ::flux::current_request [dict create verb GET headers [list] data ""]

    set full_ns ::$req_name
    namespace eval $req_name {}

    uplevel 1 $body

    if {[info exist ::flux::current_request]} {
	::flux::add-request
	unset ::flux::current_request
    }
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

    dict update ::flux::current_request headers hds {
	set hds $trim_headers
    }
}

proc data {bd} {
    set bd [subst $bd]

    puts "Data: $bd"

    dict update ::flux::current_request data dt {
	set dt $bd
    }
}
