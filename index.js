var util = require('util');

var binding = null;
try {
	binding = require(__dirname + '/build/Debug/obj.target/isc_dhclient.node');
} catch(e) {
	if(e.code == 'MODULE_NOT_FOUND')
		binding = require(__dirname + '/build/Release/obj.target/isc_dhclient.node');
	else
		console.error("ERROR: " + util.inspect(e));
}

if(binding) {
	module.exports = binding;
} else {
	console.error("Error loading node-isc-dhclient native.");
}


module.exports.newClient = function() {
  var o = binding._newClient();

  o.setLeaseCallback = function(cb) {
    return o._setLeaseCallback(function(lease) {
  //    console.log("Got lease: " + lease);
      lease = lease.replace(/\\n/g, "\\n")  
                   .replace(/\\'/g, "\\'")
                   .replace(/\\"/g, '\\"')
                   .replace(/\\&/g, "\\&")
                   .replace(/\\r/g, "\\r")
                   .replace(/\\t/g, "\\t")
                   .replace(/\\b/g, "\\b")
                   .replace(/\\f/g, "\\f");
    // remove non-printable and other non-valid JSON chars
      lease = lease.replace(/[\u0000-\u0019]+/g,""); 
      var obj = null;
      try {
        obj = JSON.parse(lease);
        // console.log("valid json:");
        // console.dir(obj);
      } catch(e) {  
        console.error("Invalid JSON in lease: " + util.inspect(e));
        cb(null);
      }
      if(obj) {
        cb(obj);
      }
    });
  }

  return o;
}
