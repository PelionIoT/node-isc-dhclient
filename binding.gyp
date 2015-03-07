{
  "targets": [
    {
      "target_name": "sixlbr_binding",

      "sources": [
        "./src/node-dhclient.cc",
		"./deps/isc-dhcp/client/dhc6.c",
		"./deps/isc-dhcp/client/clparse.c",
		"./deps/isc-dhcp/client/dhc6.c",

# common/libdhcp.a see Makefile.am
#		alloc.c bpf.c comapi.c conflex.c ctrace.c discover.c \
#		    dispatch.c dlpi.c dns.c ethernet.c execute.c fddi.c \
#		    icmp.c inet.c lpf.c memory.c nit.c ns_name.c options.c \
#		    packet.c parse.c print.c raw.c resolv.c socket.c \
#		    tables.c tr.c tree.c upf.c
#		
		
		"./deps/isc-dhcp/common/alloc.c",
		"./deps/isc-dhcp/common/bpf.c",
		"./deps/isc-dhcp/common/comapi.c",
		"./deps/isc-dhcp/common/conflex.c",
		"./deps/isc-dhcp/common/ctrace.c",
		"./deps/isc-dhcp/common/discover.c",
		"./deps/isc-dhcp/common/dispath.c",
		"./deps/isc-dhcp/common/dlpi.c",
		"./deps/isc-dhcp/common/dns.c",
		"./deps/isc-dhcp/common/etheret.c",
		"./deps/isc-dhcp/common/execute.c",
		"./deps/isc-dhcp/common/fddi.c",
		"./deps/isc-dhcp/common/icmp.c",
		"./deps/isc-dhcp/common/inet.c",
		"./deps/isc-dhcp/common/lpf.c",
		"./deps/isc-dhcp/common/memory.c",
		"./deps/isc-dhcp/common/nit.c",
		"./deps/isc-dhcp/common/ns_name.c",
		"./deps/isc-dhcp/common/options.c",
		"./deps/isc-dhcp/common/packet.c",
		"./deps/isc-dhcp/common/parse.c",
		"./deps/isc-dhcp/common/print.c",
		"./deps/isc-dhcp/common/raw.c",
		"./deps/isc-dhcp/common/resolv.c",
		"./deps/isc-dhcp/common/socket.c",
		"./deps/isc-dhcp/common/tables.c",
		"./deps/isc-dhcp/common/tr.c",
		"./deps/isc-dhcp/common/tree.c",
		"./deps/isc-dhcp/common/upf.c",

#omapip/libomapi.a see Makefile.am
#libomapi_a_SOURCES = protocol.c buffer.c alloc.c result.c connection.c \
#		     errwarn.c listener.c dispatch.c generic.c support.c \
#		     handle.c message.c convert.c hash.c auth.c inet_addr.c \
#		     array.c trace.c toisc.c iscprint.c isclib.c
#

		"./deps/isc-dhcp/omapip/protocol.c",
		"./deps/isc-dhcp/omapip/buffer.c",
		"./deps/isc-dhcp/omapip/alloc.c",
		"./deps/isc-dhcp/omapip/result.c",
		"./deps/isc-dhcp/omapip/connection.c",
		"./deps/isc-dhcp/omapip/errwarn.c",
		"./deps/isc-dhcp/omapip/listener.c",
		"./deps/isc-dhcp/omapip/dispath.c",
		"./deps/isc-dhcp/omapip/generic.c",
		"./deps/isc-dhcp/omapip/support.c",
		"./deps/isc-dhcp/omapip/handle.c",
		"./deps/isc-dhcp/omapip/message.c",
		"./deps/isc-dhcp/omapip/convert.c",
		"./deps/isc-dhcp/omapip/hash.c",
		"./deps/isc-dhcp/omapip/auth.c",
		"./deps/isc-dhcp/omapip/inet_addr.c",
		"./deps/isc-dhcp/omapip/array.c",
		"./deps/isc-dhcp/omapip/trace.c",
		"./deps/isc-dhcp/omapip/toisc.c",
		"./deps/isc-dhcp/omapip/iscprint.c",
		"./deps/isc-dhcp/omapip/isclib.c",


			

      ],

#      "include_dirs": [
#          "../deps/twlib/include"
#      ],

      "cflags": [
        "-Wall",
        "-std=c++11",
        "-fPIC",
#        "-E", # for debugging #defines

        "-I../src",
        "-I../6lbr/examples/6lbr/platform/native",
        "-I../6lbr/examples/6lbr",
        "-I../6lbr/examples/6lbr/6lbr",
        "-I../6lbr/examples/6lbr/apps/webserver",
        "-I../6lbr/examples/6lbr/apps/udp-server",
        "-I../6lbr/examples/6lbr/apps/coapserver",
        "-I../6lbr/examples/6lbr-demo/apps/udp-client",
        "-I../6lbr/examples/6lbr-demo/apps/coap",
        "-I../6lbr/examples/6lbr/apps/node-info",
        "-I../6lbr/apps/er-coap-13",
        "-I../6lbr/apps/slip-cmd",
        "-I../6lbr/apps/erbium",
        "-I../6lbr/platform/native",
        "-I../6lbr/core",
        "-I../6lbr/core/dev",
        "-I../6lbr/core/sys",
        "-I../6lbr/core/net",
        "-I../6lbr/core/net/mac",
        "-I../6lbr/core/net/ip",
        "-I../6lbr/core/net/ipv6",
        "-I../6lbr/core/net/rpl",
        "-I../6lbr/cpu/native",
        "-I../deps/twlib/include",

        "-DDEBUG=1",

        "-DCONTIKI_TARGET_NATIVE",
        "-DWITH_ARGS",
        "-DTWLIB_HAS_MOVE_SEMANTICS",  # note: requires compiler w/ C++ 11 support
        "-DAUTOSTART_ENABLE",
	    "-DWITH_COAP=13",
	    "-DREST=coap_rest_implementation",
	    "-DUIP_CONF_TCP=0",

        "-D_ERRCMN_ADD_CONSTS",
        "-DWITH_UIP6=1",
        "-DUIP_CONF_IPV6=1",
        "-DPROJECT_CONF_H=\"project-conf.h\"",
        "-DUIP_CONF_IPV6_RPL=1",
        "-DCETIC_NODE_INFO=1",
        "-D_6LBR_NODE_MODULE=1",



#        "-DCETIC_6LBR_SMARTBRIDGE=$(CETIC_6LBR_SMARTBRIDGE)",
#        "-DCETIC_6LBR_TRANSPARENTBRIDGE=$(CETIC_6LBR_TRANSPARENTBRIDGE)",
        "-DCETIC_6LBR_ROUTER=1",
#        "-DCETIC_6LBR_6LR=$(CETIC_6LBR_6LR)",
#        "-DCETIC_6LBR_ONE_ITF=1",   # I think this means one interface...
#        "-DCETIC_6LBR_LEARN_RPL_MAC=$(CETIC_6LBR_LEARN_RPL_MAC)"
      ],

      "conditions": [
        [
          "OS=='linux'", {
          "configurations" : {
            "Release" : {
	      "defines" : [ "linux" ]
            },
            "Debug" : {
              "defines" : [ "linux", "ERRCMN_DEBUG_BUILD", "PSEUDOFS_DEBUG_BUILD" ]
            }
          }
          }
        ],        
        [
          "OS=='mac'", {
            "xcode_settings": {
              "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
              "WARNING_CFLAGS": [
                "-Wno-unused-variable",
              ],
            }
          }
        ]
      ]
    },
    {
      "target_name": "action_after_build",
      "type": "none",
      "dependencies": [ "sixlbr_binding" ],
      "copies": [
        {
          "files": [ "<(PRODUCT_DIR)/sixlbr_binding.node" ],
          "destination": "<(module_path)"
        }
      ]
    }

  ]
}
