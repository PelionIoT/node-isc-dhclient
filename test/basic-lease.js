var util = require('util');
var DHCP = require('../build/Debug/isc_dhclient.node');

var client = DHCP.newClient();

console.dir(client);
console.dir(client.__proto__);

var conf = client.getConfig();

console.log("conf: " + util.inspect(conf));

client.requestLease(function(err,results){
	console.log("in callback:");
	if(err) {
		console.log("Err: " + util.inspect(err));
	} else {
		console.log("Success: " + util.inspect(results));
	}
});