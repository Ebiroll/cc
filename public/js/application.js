//<script type="text/javascript">var username=(insert JSON-encoded string here);</script>
var map;
$(function() {
	// generate unique user id
	//var userId = Math.random().toString(16).substring(2,15);
	var userId =username;
                
        //console-log("User",)
        
	var info = $('#infobox');
	var doc = $(document);

	// custom marker's icon styles
	var tinyIcon = L.Icon.extend({
		options: {
			shadowUrl: '../assets/marker-shadow.png',
			iconSize: [25, 39],
			iconAnchor:   [12, 36],
			shadowSize: [41, 41],
			shadowAnchor: [12, 38],
			popupAnchor: [0, -30]
		}
	});
	var redIcon = new tinyIcon({ iconUrl: '../assets/marker-red.png' });
	var yellowIcon = new tinyIcon({ iconUrl: '../assets/marker-yellow.png' });

	var sentData = {};

	var connects = {};
	var markers = {};
	var active = false;

	socket.on('load:coords', function(data) {
                console.log("got",data);
		if (!(data.id in connects)) {
			setMarker(data);
		}

		connects[data.id] = data;
		connects[data.id].updated = $.now(); // shothand for (new Date).getTime()
	});

	// check whether browser supports geolocation api
	if (navigator.geolocation) {
		navigator.geolocation.getCurrentPosition(positionSuccess, positionError, { enableHighAccuracy: true });
	} else {
		$('.map').text('Your browser is out of fashion, there\'s no geolocation!');
	}

	function positionSuccess(position) {
		var lat = position.coords.latitude;
		var lng = position.coords.longitude;
		var acr = position.coords.accuracy;

		// mark user's position
		var userMarker = L.marker([lat, lng], {
			icon: redIcon
		});
		// uncomment for static debug
		// userMarker = L.marker([51.45, 30.050], { icon: redIcon });

		// load leaflet map
                map = L.map('map').setView([59.4232389, 17.8295509], 13);

		L.tileLayer('https://{s}.tiles.mapbox.com/v3/olof-astrand.i90p08cm/{z}/{x}/{y}.png', {
			maxZoom: 18,
			//attribution: 'Map data &copy; <a href="http://openstreetmap.org">OpenStreetMap</a> contributors, ' +
			//	'<a href="http://creativecommons.org/licenses/by-sa/2.0/">CC-BY-SA</a>, ' +
			//	'Imagery © <a href="http://mapbox.com">Mapbox</a>',
			id: 'chicken.map'
		}).addTo(map);


		L.marker([59.4232389, 17.8295509]).addTo(map)
			.bindPopup("<b>Hello world!</b><br />I am an egg").openPopup();

		L.circle([59.4232389, 17.8295509], 50, {
			color: 'orange',
			fillColor: '#f03',
			fillOpacity: 0.5
		}).addTo(map).bindPopup("I am a circle.");

                
		userMarker.addTo(map);
		userMarker.bindPopup('<p>You are there! Your are ' + userId + '</p>').openPopup();
                
                var popup = L.popup();

		function onMapClick(e) {
			popup
				.setLatLng(e.latlng)
				.setContent("You clicked the map at " + e.latlng.toString())
				.openOn(map);
		}

		map.on('click', onMapClick);

		var emit = $.now();
		// send coords on when user is active
		doc.on('mousemove', function() {
			active = true;

			sentData = {
				id: userId,
				active: active,
				coords: [{
					lat: lat,
					lng: lng,
					acr: acr
				}]
			};

			if ($.now() - emit > 30) {
				socket.emit('send:coords', sentData);
				emit = $.now();
			}
		});
	}

	doc.bind('mouseup mouseleave', function() {
		active = false;
	});

	// showing markers for connections
	function setMarker(data) {
		for (var i = 0; i < data.coords.length; i++) {
			var marker = L.marker([data.coords[i].lat, data.coords[i].lng], { icon: yellowIcon }).addTo(map);
			marker.bindPopup('<p>' + data.id + ' is here!</p>');
			markers[data.id] = marker;
		}
	}

	// handle geolocation api errors
	function positionError(error) {
		var errors = {
			1: 'Authorization fails', // permission denied
			2: 'Can\'t detect your location', //position unavailable
			3: 'Connection timeout' // timeout
		};
		showError('Error:' + errors[error.code]);
	}

	function showError(msg) {
		info.addClass('error').text(msg);

		doc.click(function() {
			info.removeClass('error');
		});
	}

	// delete inactive users every 15 sec
	setInterval(function() {
		for (var ident in connects){
			if ($.now() - connects[ident].updated > 150000) {
				delete connects[ident];
				map.removeLayer(markers[ident]);
			}
		}
	}, 150000);
});