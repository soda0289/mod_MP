//Music UI context object
function music_ui(domain) {
	this.domain = domain;

	//Time to wait before requesting data again
	this.refresh_interval = 1000;


	this.init = function (){	
		var music_ui_ctx = this;
		
		var music_window =  document.getElementById("right");
		

		//Defualt playlist
		var obj = {"name" : "songs", "index" : "song_id"};

		var parameters = new query_parameters("songs",obj);
		parameters.num_results = 1000;

		parameters.sort_by = 
		[{
			"friendly_name":"album_name", 
			"order" : "+"
		},{
			"friendly_name":"disc_no",
			"order" : "+"
		},{
			"friendly_name":"track_no",
			"order" : "+"
		}];
		
		var default_playlist = new playlist(domain, parameters);
		
		//player
		this.player_if = new player(default_playlist, music_ui_ctx);
		music_window.appendChild(this.player_if.div);
		//Music Browser/Playlist Creator
		this.browser_if = new browser(music_window, music_ui_ctx);
		//Playlist tabs
		this.playlist_tabs_if = new playlist_tabs(music_window,music_ui_ctx);
		//Resize playlist tabs
		this.playlist_tabs_if.div.style.top = (parseInt(this.player_if.div.style.height,10) + parseInt(window.getComputedStyle(this.browser_if.div).height,10)) + "px";
		//Default playlist
		this.playlist_tabs_if.add_tab("all", default_playlist);
		//Statusbar
		this.status_bar = new status_bar(music_ui_ctx);
		music_window.appendChild(this.status_bar.div);
	};
}




function loadUI(){

	var login_window = new floating_box("login_box", "50%","50%",300 + "px",150 + "px");
	login_window.center();
	
	var server_url = document.createElement("input");
	server_url.className = "textbox";
	server_url.type = "text";
	server_url.value = "mp.attiyat.net";
	
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
	ok_button.id = "ok_button";
	ok_button.class = "ok_button";
	ok_button.onclick = function(event){
		var server_url_div = document.getElementById("login_server_url");
		var server_url = server_url_div.getElementsByClassName("textbox")[0];
		var login_box = document.getElementById("login_box");

		var music_ui_ctx = new music_ui(server_url.value);
		music_ui_ctx.init();

		login_box.parentNode.removeChild(login_box);
	};
	login_window.appendChild(ok_button);
	login_window.show();
}
//note to self
//when you set a varible to a function
//you must not include parenthises
//This will run that function and not pass
//the function as a pointer
window.onload = loadUI;
