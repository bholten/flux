namespace eval ::___flux::main {
    variable current_workspace
    variable workspaces {}

    proc create_workspace {workspace} {
	set ::___flux::main::current_workspace $workspace
	lappend ::___flux::main::workspaces $workspace

	namespace eval ::___flux::main::${workspace} {
	    variable current_request
	    variable requests

	    if {![info exists requests]} {
		set requests {}
	    }

	    proc add_request {} {
		set ws $::___flux::main::current_workspace
		set reqs ::___flux::main::${ws}::requests
		lappend $reqs $::___flux::main::current_request
	    }

	    proc create_request {verb url body} {
		set ws $::___flux::main::current_workspace

		set req_name "GET/$url"
		set ::___flux::main::current_request [dict create url $url verb GET headers [list] data ""]

		set full_ns ::___flux::main::${ws}::${req_name}
		namespace eval $full_ns {}
		namespace eval ${full_ns}::config {}
		namespace eval ::___flux::main $body

		add_request
		
		if {[info exist ::___flux::main::current_request]} {
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
    }
}

proc workspace {name} {
    ::___flux::main::create_workspace $name
}

proc config {body} {
    set cw $::___flux::main::current_workspace
    namespace eval ::___flux::main::${cw}::config [list $body]
}

proc http_req {verb url body} {
    set cw $::___flux::main::current_workspace
    ::___flux::main::${cw}::create_request $verb $url $body
}

proc GET {url body} {
    http_req GET $url $body
}

proc POST {url body} {
    http_req POST $url $body
}

proc PUT {url body} {
    http_req PUT $url $body
}

proc DELETE {url body} {
    http_req DELETE $url $body
}

proc PATCH {url body} {
    http_req PATCH $url $body
}

proc OPTIONS {url body} {
    http_req OPTIONS $url $body
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

    dict update ::___flux::main::current_request headers hds {
	set hds $trim_headers
    }
}

proc data {bd} {
    set bd [subst $bd]

    dict update ::___flux::main::current_request data dt {
	set dt $bd
    }
}
