{
  "targets": [
    {
      "target_name": "isc-dhclient",

      "sources": [
      	"./src/binding.cc",
      	"./src/error-common.cc",
        "./src/dhclient-cfuncs.c",
        "./src/overlay-errwarn.c",
        "./src/overlay-clparse.c",
        "./src/overlay-parse.c",


# common/libdhcp.a see Makefile.am
#		alloc.c bpf.c comapi.c conflex.c ctrace.c discover.c \
#		    dispatch.c dlpi.c dns.c ethernet.c execute.c fddi.c \
#		    icmp.c inet.c lpf.c memory.c nit.c ns_name.c options.c \
#		    packet.c parse.c print.c raw.c resolv.c socket.c \
#		    tables.c tr.c tree.c upf.c
#

		"./deps/isc-dhcp/client/dhc6.c",
#		"./deps/isc-dhcp/client/clparse.c",
		"./deps/isc-dhcp/common/alloc.c",
		"./deps/isc-dhcp/common/bpf.c",
		"./deps/isc-dhcp/common/comapi.c",
		"./deps/isc-dhcp/common/conflex.c",
		"./deps/isc-dhcp/common/ctrace.c",
		"./deps/isc-dhcp/common/discover.c",
		"./deps/isc-dhcp/common/dispatch.c",
		"./deps/isc-dhcp/common/dlpi.c",
		"./deps/isc-dhcp/common/dns.c",
		"./deps/isc-dhcp/common/ethernet.c",
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
#		"./deps/isc-dhcp/common/parse.c",
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
#		"./deps/isc-dhcp/omapip/errwarn.c",
		"./deps/isc-dhcp/omapip/listener.c",
		"./deps/isc-dhcp/omapip/dispatch.c",
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

      "include_dirs": [
          "deps/twlib/include",
          "deps/isc-dhcp/bind/include",
          "deps/isc-dhcp/includes",
          "deps/isc-dhcp",
          "deps/isc-dhcp/bind/bind-expanded-tar",
          "<!(node -e \"require('nan')\")"
      ],

      "cflags": [
        "-Wall",
        "-std=c++11",
        "-fPIC",
#        "-E", # for debugging #defines
        "-I../src",
        "-I../deps/isc-dhcp/bind/include",


		"-DDHCPv6=1",
        "-D_ERRCMN_ADD_CONSTS",
        "-DLOCALSTATEDIR=\"/NOVAR\"",

# these two provide heavy debugging for the semaphore / queing stuff
#        "-DDEBUG_TW_CIRCULAR_H",
#        "-D_TW_SEMA2_HEAVY_DEBUG",

        # out of config.h or site.h
#        "-DNSUPDATE"


      ],

      "conditions": [
        [
          "OS=='linux'", {
          "doit" : '<!(deps/install-deps.sh 2>&1 > /tmp/install_deps.log)',  # doit means nothing - but this forces this script execute
          "configurations" : {
            "Release" : {
	      		"defines" : [ "linux" ],
           		"ldflags" : [
            		"-L../deps/isc-dhcp/bind/lib",
            		## education: http://stackoverflow.com/questions/14889941/link-a-static-library-to-a-shared-one-during-build
            		"-Wl,-whole-archive ../deps/isc-dhcp/bind/lib/libdns.a ../deps/isc-dhcp/bind/lib/libisc.a -Wl,-no-whole-archive"
		        ],
                "devDependencies" : [
                    "https://github.com/WigWagCo/greaseLog.git#nan"
                ],
            },
            "Debug" : {
              	"defines" : [ "linux", "ERRCMN_DEBUG_BUILD", "NODE_ISCDHCP_DEBUG" ],
              	"cflags" : [
              	    "-DDEBUG=1",
                    "-D_GNU_SOURCE",
                    "-ggdb" # for debugging
              	],
           		"ldflags" : [
            		"-L../deps/isc-dhcp/bind/lib",
            		## education: http://stackoverflow.com/questions/14889941/link-a-static-library-to-a-shared-one-during-build
            		"-Wl,-whole-archive ../deps/isc-dhcp/bind/lib/libdns.a ../deps/isc-dhcp/bind/lib/libisc.a -Wl,-no-whole-archive"
		        ],
                "devDependencies" : [
                    "https://github.com/WigWagCo/greaseLog.git#nan"
                ]
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
    }

#    ,
#    {
#      "target_name": "action_after_build",
#      "type": "none",
#      "dependencies": [ "isc_dhclient" ],
#      "copies": [
#        {
#          "files": [ "<(PRODUCT_DIR)/isc_dhclient.node" ],
#          "destination": "<(module_path)"
#        }
#      ]
#    }

  ]
}
