var util = require('util');

var nativelib = null;
try {
	nativelib = require('../build/Release/isc_dhclient.node');
} catch(e) {
	if(e.code == 'MODULE_NOT_FOUND')
		nativelib = require('../build/Debug/isc_dhclient.node');
	else
		console.error("Error in nativelib [debug]: " + e + " --> " + e.stack);
}
var DHCP = nativelib;


var client = DHCP.newClient();

console.dir(client);
console.dir(client.__proto__);

var conf = client.getConfig();

conf.interfaces = [];
conf.interfaces.push("eth0");
// config_options is essentially the same as dhclient.conf
// note - using a '\n' will not work here... use a literal new line as below.

//
// do_forward_updates false;\

conf.config_options = 
"timeout 60;\
MISTAKE \
do-forward-updates false;\
option rfc3442-classless-static-routes code 121 = array of unsigned integer 8;"
client.setConfig(conf);

console.log("conf: " + util.inspect(conf));

client.start();

client.requestLease(function(err,results){
	console.log("in callback:");
	if(err) {
		console.log("Err: " + util.inspect(err));
	} else {
		console.log("Success: " + util.inspect(results));
	}
	client.shutdown(function(err){
		if(err)
			console.log("Err on shutdown: " + util.inspect(err));
		else
			console.log("dhclient shutdown complete.");
	});	
});