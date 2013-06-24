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
	
	music_ui_ctx.browser_if = new browser(music_window, music_ui_ctx);
	//music_ui_ctx.browser_if.add_table([{"header" : "Artist","friendly_name" : "artist_name"}], "artists");
	//music_ui_ctx.browser_if.add_table([{"header" : "Albums","friendly_name" : "album_name"}], "albums");
	
	music_ui_ctx.playlist_tabs_if = new playlist_tabs(music_window,music_ui_ctx);
	//Resize playlist tabs
	music_ui_ctx.playlist_tabs_if.div.style.height = (parseInt(document.documentElement.clientHeight, 10) - 20 - (parseInt(music_ui_ctx.player_if.div.style.height,10) + parseInt(window.getComputedStyle(music_ui_ctx.browser_if.div).height,10))) + "px";
	
	music_ui_ctx.playlist_tabs_if.add_tab("all", default_playlist);
}


function loadUI(){

	var login_window = new floating_box("login_box", ((parseInt(document.documentElement.clientHeight, 10) / 2) - 75) + "px", ((parseInt(document.documentElement.clientWidth, 10) / 2) - 150) + "px",300 + "px",150 + "px");
	var server_url = document.createElement("input");
	server_url.className = "textbox";
	server_url.type = "text";
	
	var server_url_div = document.createElement("div");
	server_url_div.id = "login_server_url";
	server_url_div.innerHTML = "Server URL: ";
	server_url_div.appendChild(server_url);
	login_window.appendChild(server_url_div);
	
	
	
	var username = document.createElement("input");
	username.className = "textbox";
	username.type = "text";
	
	var username_div = document.createElement("div");
	username_div.id = "login_username";
	username_div.innerHTML = "User name: ";
	username_div.appendChild(username);
	login_window.appendChild(username_div);
	
	
	var password = document.createElement("input");
	password.className = "textbox";
	password.type = "password";
	
	var password_div = document.createElement("div");
	password_div.id = "login_password";
	password_div.innerHTML = "Passowrd: ";
	password_div.appendChild(password);
	login_window.appendChild(password_div);
	
	
	
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
		login_box.parentNode.removeChild(login_box);
	}
	login_window.appendChild(ok_button);
	login_window.show();
}
//note to self
//when you set a varible to a function
//you must not include parenthises
//This will run that function and not pass
//the function as a pointer
window.onload = loadUI;