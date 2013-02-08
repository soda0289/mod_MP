function unhighlight_song(music_ui_ctx){
	if(music_ui_ctx.playing_index >= 0){
		var old_song = document.getElementById("song_" + music_ui_ctx.songs[music_ui_ctx.playing_index].song_id);
		old_song.style.backgroundColor = music_ui_ctx.song_backgroundColor;
	}
}

function shuffle_playlist(music_ui_ctx){
	
	var shuffle_text_elem = document.createElement("span");
	
	music_ui_ctx.player_status.appendChild(shuffle_text_elem);
	
	music_ui_ctx.shuffled_playlist = new Array();
	music_ui_ctx.shuffled_playlist[0] = music_ui_ctx.songs[0];
	for(var i = 1;i < music_ui_ctx.songs.length; i++){
		//Random number from [0, i]
		
		var random = Math.floor(Math.random() * (i + 1));
		music_ui_ctx.shuffled_playlist[i] = music_ui_ctx.shuffled_playlist[random];
		music_ui_ctx.shuffled_playlist[random] = music_ui_ctx.songs[i];
	}
	
	music_ui_ctx.unshuffled_playlist = music_ui_ctx.songs;
	music_ui_ctx.songs = music_ui_ctx.shuffled_playlist;
	
	shuffle_text_elem.innerHTML = "shuffled";
}

function update_time(music_ui_ctx){
	var current_seconds = ( parseInt(music_ui_ctx.audio_ele.currentTime % 60) < 10) ? "0" + parseInt(music_ui_ctx.audio_ele.currentTime % 60):parseInt(music_ui_ctx.audio_ele.currentTime % 60);
	var duration_seconds = ( parseInt(music_ui_ctx.audio_ele.duration % 60) < 10) ? "0" + parseInt(music_ui_ctx.audio_ele.duration % 60):parseInt(music_ui_ctx.audio_ele.duration % 60);
	
	music_ui_ctx.time_elem.innerHTML = parseInt(music_ui_ctx.audio_ele.currentTime / 60) + ":" +  current_seconds  + "/" + parseInt(music_ui_ctx.audio_ele.duration /60) + ":" + duration_seconds;
	
	
	
	
}

function get_next_song_index(music_ui_ctx){
	music_ui_ctx.playing_index += 1;
	if(music_ui_ctx.playing_index === music_ui_ctx.songs.length){
		music_ui_ctx.playing_index = 0;
	}
	return music_ui_ctx.playing_index;
}

//Music UI context object
function music_ui(songs_query, artists_query, albums_query) {
	this.songs_query = songs_query;
	this.artists_query = artists_query;
	this.albums_query = albums_query;
	
	//Create array that links to each query
	//this is only used in the for statement
	this.queries = new Array(this.songs_query, this.artists_query, this.albums_query);
	
	//Array that hold the current UI listings
	this.songs = new Array();
	this.albums = new Array();
	this.artists = new Array();
	this.sources = new Array();
	this.decoding_job = new Array();
	
	
	//Audio Element
	this.audio_ele = new Audio;
	this.audio_ele.addEventListener('timeupdate', 
	function (music_ui_ctx) {
		return function(e){
			update_time(music_ui_ctx);
		}
	}(this), false);
	this.audio_ele.addEventListener("progress", 
	function(music_ui_ctx) {
		return function(event){
			if(this.buffered && this.duration){
				music_ui_ctx.buffer_elem.innerHTML = "Buffering:" + parseInt(this.buffered.end(0) / this.duration * 100) + "%";
			}
		}
	}(this),false);
	this.audio_ele.addEventListener("loadstart", 
			function(music_ui_ctx) {
				return function(event){
					music_ui_ctx.buffer_elem.innerHTML = "Starting to load Audio";
					music_ui_ctx.playing = 1;
					this.play();
				}
	}(this),false);
	this.audio_ele.addEventListener("loaddata", 
			function(music_ui_ctx) {
				return function(event){
					music_ui_ctx.buffer_elem.innerHTML = "Loading Audio";
					music_ui_ctx.playing = 1;
					this.play();
				}
	}(this),false);
	this.audio_ele.addEventListener("loadmetadata", 
			function(music_ui_ctx) {
				return function(event){
					music_ui_ctx.buffer_elem.innerHTML = "Loaded metadata";
					music_ui_ctx.playing = 1;
					this.play();
				}
	}(this),false);
	this.audio_ele.addEventListener("canPlay", 
			function(music_ui_ctx) {
				return function(event){
					music_ui_ctx.buffer_elem.innerHTML = "Loaded enough to play";
				}
	}(this),false);
	this.audio_ele.addEventListener("canPlayThru", 
			function(music_ui_ctx) {
				return function(event){
					music_ui_ctx.buffer_elem.innerHTML = "Done loading. 100% downloaded";
				}
	}(this),false);
	//Play next song when finished current
	this.audio_ele.addEventListener('ended', function(music_ui_ctx){
			return function(event){
				music_ui_ctx.next_button.click();
			}
	}(this), false);
	

	
	//Is playing audio
	this.playing = 0;
	
	//Info about playing song
	this.song_backgroundColor;
	this.playing_index = -1;
	this.next_to_play = null;
	
	//Setup song table
	this.songs_div = document.getElementById("songs");
	this.songs_div.addEventListener("scroll",
	function(music_ui_ctx){
		return function(event){
			console.log(music_ui_ctx.songs_div.clientHeight - music_ui_ctx.songs_div.scrollTop);
		}
	}(this),false);
	this.songs_table = document.getElementById("songs_table");
	
	this.create_songs_table = function(){
		this.songs_table = document.createElement('table');
		this.songs_table.id = "songs_table";
		this.songs_table.style.width = "100%";
		this.songs_table.style.borderSpacing = "2px 2px";
		//music_ui_ctx.songs_table.borderCollapse = "Collapse";

		this.songs_div.appendChild(this.songs_table);
	}
	
	if(this.songs_table == null){
		this.create_songs_table();
	}
	
	
	//Status boxes
	this.player_status = document.getElementById("player_status");
	
	this.time_elem = document.createElement("span");
	this.player_status.appendChild(this.time_elem);
	this.buffer_elem = document.createElement("span");
	this.player_status.appendChild(this.buffer_elem);
	this.shuffled_elem = document.createElement("span");
	this.player_status.appendChild(this.shuffled_elem);
	

	this.song_info_elem = document.getElementById("song_info");
	
	//Setup Buttons
	this.play_button = document.getElementById("play_pause");
	this.play_button.onclick = function (music_ui_ctx){
		return function(event){
			//Play first song
			music_ui_ctx.playing_index = 0;
			play_song(music_ui_ctx.songs[0], music_ui_ctx);
		}
	}(this);
	this.previous_button = document.getElementById("previous");
	this.previous_button.onclick = function (music_ui_ctx){
		return function(event){
			unhighlight_song(music_ui_ctx);
			music_ui_ctx.playing = 0;
			
			music_ui_ctx.playing_index -= 1;
			if(music_ui_ctx.playing_index< 0){
				music_ui_ctx.playing_index = music_ui_ctx.songs.length -1;
			}
			play_song(music_ui_ctx.songs[music_ui_ctx.playing_index], music_ui_ctx);
		}
	}(this);
	this.next_button = document.getElementById("next");
	this.next_button.onclick = function (music_ui_ctx){
		return function(event){
			//play next song
			play_song(music_ui_ctx.songs[get_next_song_index(music_ui_ctx)], music_ui_ctx);
		}
	}(this);
	this.shuffle = document.getElementById("shuffle");
	this.shuffle.onclick = function (music_ui_ctx){
		return function(event){
			shuffle_playlist(music_ui_ctx);
		}
	}(this);
}


function loadUI(){
	
	//Create queries for Songs, Artists, and Albums
	var songs_query = new music_query("mp.attiyat.net", 750, "songs", "song_title", 0, 0, 0,print_song_table);
	var artists_query = new music_query("mp.attiyat.net", 100, "artists", "artist_name", 0, 0, 0,print_artists);
	var albums_query = new music_query("mp.attiyat.net", 100, "albums", "album_name", 0, 0, 0,print_albums);
	
	//Load music ui
	var music_ui_ctx = new music_ui(songs_query, artists_query, albums_query);
	
	load_queries(music_ui_ctx);
}
//note to self
//when you set a varible to a function
//you must not include parenthises
//This will run that function and not pass
//the function as a pointer
window.onload = loadUI;