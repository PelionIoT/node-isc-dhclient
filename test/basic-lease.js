var DHCP = require('../build/Debug/isc_dhclient.node');

var client = DHCP.newClient();

console.dir(client);
console.dir(client.__proto__);

