function unhighlight_song(music_ui_ctx){
	if(music_ui_ctx.playing_index >= 0){
		var old_song = document.getElementById("song_" + music_ui_ctx.songs[music_ui_ctx.playing_index].song_id);
		old_song.style.backgroundColor = music_ui_ctx.song_backgroundColor;
	}
}

//Music UI context object
function music_ui() {

}

function create_inital_queries(domain){	
	//Load music ui
	var music_ui_ctx = new music_ui();
	music_ui_ctx.domain = domain;
	
	var music_window =  document.getElementById("right");
	

	//Defualt playlist
	var parameters = new query_parameters("songs");
	parameters.num_results = 1000;
	parameters.sort_by = "song_title";
	
	var default_playlist = new playlist(domain, parameters);
	
	//player
	var player_if = new player(default_playlist, music_ui_ctx);
	music_window.appendChild(player_if.div);
	
	
	var artist_album_if = new artist_album_browser(music_window,music_ui_ctx);
	
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
	server_url.id = "server_url"
	server_url.type = "text";
	
	var server_url_div = document.createElement("div");
	server_url_div.innerHTML = "Server URL: ";
	server_url_div.appendChild(server_url);
	login_box.appendChild(server_url_div);
	
	
	
	var username = document.createElement("input");
	username.type = "text";
	
	var username_div = document.createElement("div");
	username_div.innerHTML = "User name: ";
	username_div.appendChild(username);
	login_box.appendChild(username_div);
	
	
	var password = document.createElement("input");
	password.type = "password";
	
	var password_div = document.createElement("div");
	password_div.innerHTML = "Passowrd: ";
	password_div.appendChild(password);
	login_box.appendChild(password_div);
	
	
	
	var ok_button = document.createElement("input");
	ok_button.type = "button";
	ok_button.value = "OK";
	ok_button.id = "ok_button"
	ok_button.onclick = function(event){
		var server_url = document.getElementById("server_url");
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