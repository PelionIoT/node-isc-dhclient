var util = require('util');

var DHCP = require('../index.js');


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
do-forward-updates false;\
option rfc3442-classless-static-routes code 121 = array of unsigned integer 8;"
client.setConfig(conf);

//console.log("conf: " + util.inspect(conf));

client.setLeaseCallback(function(lease) {
	if(lease) {
		console.log("got lease: ");
		console.dir(lease);
	} else {
		console.log("error in parsing lease.");
	}
});

client.start();

var lease = { interface: 'eth0',
  fixed_address: '10.0.0.8',
  options:
   { 'subnet-mask': '255.255.254.0',
     routers: '10.0.0.1',
     'dhcp-lease-time': '86400',
     'dhcp-message-type': '5',
     'domain-name-servers': '10.0.0.1',
     'dhcp-server-identifier': '10.0.0.1' },
  renew: '1 2015/08/10 04:49:45;',
  rebind: '1 2015/08/10 15:08:44;',
  expire: '1 2015/08/10 18:08:44;' };
client.setCurrentLease(lease);

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