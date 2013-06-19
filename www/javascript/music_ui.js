function unhighlight_song(music_ui_ctx){
	if(music_ui_ctx.playing_index >= 0){
		var old_song = document.getElementById("song_" + music_ui_ctx.songs[music_ui_ctx.playing_index].song_id);
		old_song.style.backgroundColor = music_ui_ctx.song_backgroundColor;
	}
}

//Music UI context object
function music_ui(domain) {
	this.domain = domain;
}

function create_inital_queries(domain){	
	//Load music ui
	var music_ui_ctx = new music_ui(domain);
	
	var music_window =  document.getElementById("right");
	

	//Defualt playlist
	var parameters = new query_parameters("songs");
	parameters.num_results = 1000;
	parameters.sort_by = "song_title";
	
	var default_playlist = new playlist(domain, parameters);
	
	//player
	music_ui_ctx.player_if = new player(default_playlist, music_ui_ctx);
	music_window.appendChild(music_ui_ctx.player_if.div);
	
	
	//music_ui_ctx.artist_album_if = new artist_album_browser(music_window,music_ui_ctx);
	
	music_ui_ctx.browser_if = new browser(music_window, music_ui_ctx);
	//music_ui_ctx.browser_if.add_table([{"header" : "Artist","friendly_name" : "artist_name"}], "artists");
	//music_ui_ctx.browser_if.add_table([{"header" : "Albums","friendly_name" : "album_name"}], "albums");
	
	music_ui_ctx.playlist_tabs_if = new playlist_tabs(music_window,music_ui_ctx);
	
	music_ui_ctx.playlist_tabs_if.add_tab("all", default_playlist);
}


function loadUI(){
	var login_box = document.createElement("div");
	login_box.id = "login_box";
	login_box.style.position = "absolute";
	login_box.style.top = "50%";
	login_box.style.left = "50%";
	login_box.style.width = "300px";
	login_box.style.height = "150px";
	login_box.style.backgroundColor = "white";
	

	
	var server_url = document.createElement("input");
	server_url.className = "textbox";
	server_url.type = "text";
	
	var server_url_div = document.createElement("div");
	server_url_div.id = "login_server_url";
	server_url_div.innerHTML = "Server URL: ";
	server_url_div.appendChild(server_url);
	login_box.appendChild(server_url_div);
	
	
	
	var username = document.createElement("input");
	username.className = "textbox";
	username.type = "text";
	
	var username_div = document.createElement("div");
	username_div.id = "login_username";
	username_div.innerHTML = "User name: ";
	username_div.appendChild(username);
	login_box.appendChild(username_div);
	
	
	var password = document.createElement("input");
	password.className = "textbox";
	password.type = "password";
	
	var password_div = document.createElement("div");
	password_div.id = "login_password";
	password_div.innerHTML = "Passowrd: ";
	password_div.appendChild(password);
	login_box.appendChild(password_div);
	
	
	
	var ok_button = document.createElement("input");
	ok_button.type = "button";
	ok_button.value = "OK";
	ok_button.id = "ok_button"
	ok_button.class = "ok_button";
	ok_button.onclick = function(event){
		var server_url_div = document.getElementById("login_server_url");
		var server_url = server_url_div.getElementsByClassName("textbox")[0];
		var login_box = document.getElementById("login_box");
		create_inital_queries(server_url.value);
		login_box.style.visibility = "hidden";
	}
	login_box.appendChild(ok_button);
	
	
	
	
	
	var body = document.getElementById("body");
	body.appendChild(login_box);
}
//note to self
//when you set a varible to a function
//you must not include parenthises
//This will run that function and not pass
//the function as a pointer
window.onload = loadUI;