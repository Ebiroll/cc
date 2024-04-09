
var express = require('express');
var app = express();
app.use(express.urlencoded({extended: true}));
app.use(express.json()); // To parse the incoming requests with JSON payloads
var auth = require("basic-auth");

// Old style
//var app = require('express')()
//  , server = require('http').createServer(app)
//  , io = require('socket.io').listen(server);

var port = 10180;

//var port = 443;
 
const http = require('http');
const https = require('https');
const fs = require('fs');
const path = require('path');
const socketio = require('socket.io');



var server;
var io;

// Check if running in production
const isProduction = process.env.NODE_ENV === 'production';

if (isProduction) { // 
    // Production environment - Use HTTPS
    const certOptions = {
        key: fs.readFileSync(path.resolve('server.key')),
        cert: fs.readFileSync(path.resolve('server.cert')),
        // Include CA chain if necessary
    };
    
    server = https.createServer(certOptions, app);
} else {
    // Development or other non-production environment - Use HTTP
    server = http.createServer(app);
}

io = socketio(server);

server.listen(process.env.PORT || port, () => {
    console.log(`Server is running on port ${server.address().port}`);
});


//var server = require('http').createServer(app);
//var io = require('socket.io')(server);
//server.listen(process.env.PORT || port);
//console.log('Server is running ');


var morgan = require('morgan');
var users = require('./users/server.js');
var chickens = require('./chickens/server.js');


app.use(morgan("dev"));

// Authenticator
// http://blog.modulus.io/nodejs-and-express-basic-authentication


var passport = require('passport');
var BasicStrategy = require('passport-http').BasicStrategy;

app.use(express.static('js'));
        
passport.use(new BasicStrategy(
  function(username, password, done) {
    console.log("AUTH!!!");
      // username.valueOf() === 'olle' &&
      
    users.usersAdd(username);   
      
    if (password.valueOf() === 'pip')
      return done(null, true);
    else
      return done(null, false);
  }
));

app.use(require('express-session')({ 
  secret: 'Enter your secret key',
  resave: true,
  saveUninitialized: true
}));

app.use(passport.initialize());
app.use(passport.session());

app.use('/assets',express.static(__dirname + '/public/assets'));

app.get('/js/application2.js', passport.authenticate('basic', { session: false }), (req, res) => {
  const credentials = auth(req);
  const username = credentials.name; // Now 'username' will just have the 'name' part
  console.log("USERNAME===============",username);


  fs.readFile('public/js/application2.js', 'utf8', (error, data) => {
    if (error) {
      console.error(error);
      return res.sendStatus(500); // Send 500 Internal Server Error
    }
    // Dynamically prepend the username variable to the script
    const customScript = `var username = "${username}";\n${data}`;
    res.type('application/javascript').send(customScript);
  });
});


app.use('/js',express.static(__dirname + '/public/js'));

app.use('/lib',express.static(__dirname + '/public/lib'));

app.use('/img',express.static(__dirname + '/public/img'));

app.use('/css',express.static(__dirname + '/public/css'));

app.use('/fonts',express.static(__dirname + '/public/fonts'));


app.use('/index_files',express.static(__dirname + '/public/index_files'));

app.get('/chickens.html', function(req,res) {
  res.sendFile('chickens.html',{ root: 'public' });
});


function testMe(req,res) {

  res.sendFile('index.html',{ root: 'public' });
};

app.get('/result.html', function(req,res) {
  res.sendFile('result.html',{ root: 'public' });
});


app.get('/lmap.html', function(req,res) {
  res.sendFile('lmap.html',{ root: 'public' });
});

app.get('/gmap.html', function(req,res) {
    res.sendFile('gmap.html',{ root: 'public' });
});
  

app.get('/users/users.html', function(req,res) {
  res.sendFile('users/users.html',{ root: 'public' });
});

// app.use(express.static(__dirname + '/users/users.html'));


app.get('/js/user.js',passport.authenticate('basic', { session: false }),function(req,res) {
 var user = auth(req);
 var response;
 response = "var username= \"" + user.name + "\";";
 res.send(response);
});


app.get('/index.html' , passport.authenticate('basic', { session: false }),testMe);

app.get('/' , passport.authenticate('basic', { session: false }),testMe);


// app.use('/',passport.authenticate('basic', { session: false }),testMe);


app.get("/about", function(request, response) {
  response.writeHead(200, { "Content-Type": "text/plain" });
  response.end("Welcome to the about page!");
});

// delete to see more logs from sockets
// io.set('log level', 2);
// io.set('log level', 3);

io.on('connection', function (client) {
    
    console.log("connected");
    chickens.serveChickens(client);
    
	  client.on('send:coords', function (data) {
                console.log("savePositions",data);
                users.savePosition(data,client);
                
                users.servePosition(client);
  	});
});



// Not necessary??
app.set('json spaces', 0);

app.post("/users/records.json", (req, res)=>{
  users.usersPost(req, res);
});

app.get("/users/records.json", (req, res)=>{
  users.usersGet(req, res);
});



app.post("/users/positions.json",  (req, res)=>{
  //var postData = req.body;
  users.positionPost(req, res);
});


app.post("/chickens/records.json", (req, res)=>{
  chickens.chickensPost(req, res);
});

app.get("/chickens/records.json", (req, res)=>{
  chickens.chickensGet(req, res);
});

//app.use('/chickens',express.static(__dirname + '/public/chickens'));

//app.listen(port);
//server.listen(port);

console.log('Your server goes on localhost:' + port);