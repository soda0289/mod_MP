

function player(playlist, music_ui_ctx){
	this.domain = music_ui_ctx.domain;
	
	//Playing index of songs in playlist
	this.playing_index = 0;
	
	//Setup player div and set playlist
	this.player_div = document.createElement("div");
	this.player_div.style.height = "66px";
	this.player_div.id = "player";
	this.player_div.style.fontSize = "14";
	
	this.playlist = playlist;
	
	//Create decoding jobs array
	this.decoding_jobs = [];
	
	this.audio_obj = new audio_obj(this, music_ui_ctx);
	
	this.find_playable_source = function(song, domain){
		
		if(typeof (song) === "undefined"){
			console.error("ERROR: \nPlayer (find_playable_source): song is undefined");
			return;
		}
		
		var sources_query_loaded = function(player){
			return function (results){
		
				if (results.length > 0){
					//Copy sources to song
					song.sources = results;
	
					//Is audio playing
					if(player.audio_obj.playing === 0 && player.audio_obj.next_to_play !== null){
						player.audio_obj.play_song(song);
					}
					
				}else{
					//Decode song no source avalible
					new decoding_job(song, player);
				}
			};
		}(this);
		
		
		if(!('sources' in song) || song.sources.length < 1){
			//song has no sources
			//Fetch sources
			var sources_parameters = new query_parameters("sources");
			sources_parameters.source_type = "ogg";
			sources_parameters.song_id.push(song.song_id);
			
			var sources_query = new music_query(domain, sources_parameters, sources_query_loaded);
			sources_query.song = song;
			sources_query.player = this;
		
			sources_query.load();
			
			return 0;
		}else{
			return song.sources.length;
		}

	};
	
	this.decode_next_songs = function(num_songs){
		for(var i = 1;i <= num_songs; i++){
			var song_next = this.playlist.songs[(this.playing_index + i)%this.playlist.songs.length];
			if(typeof song_next !== "undefined"){
				this.find_playable_source(song_next, music_ui_ctx.domain);
			}else{
				break;
			}
		}
	};

	//Unhighlight playing song Hilight song
	//Update playing index
	this.change_song = function (song){
		
		if(typeof (song) === "undefined"){
			console.error("ERROR: \nPlayer (change_song): song is undefined");
			return;
		}
		
		//Check if we are displaying search results
		if(typeof (this.playlist.search_results) === "undefined"){
			//unhighlight current song
			//song is equal to the playing index for the array of songs
			if(this.playing_index !== undefined && this.playing_index >= 0 && this.playing_index < this.playlist.songs.length){
				var current_song;
				current_song = this.playlist.songs[this.playing_index];
				this.playlist.songs_table.deselect_row(current_song.table_index);
			}
		
		
			//Set playing index
			if(this.playlist.shuffled === true){
				//Check if song shuffled index is undefined
				if(typeof(song.shuffled_index) === "undefined"){
					//This can happen when the user has the playlist shuffled
					//and clicks on a song after changing table views(search or sort)
					console.error("ERROR: Player (change_song): Song doesn't have shuffled index. Shuffled index is being relyed on the set playing index.");
				}
				this.playing_index = song.shuffled_index;
			}else{
				if(typeof(song.table_index) === "undefined"){
					console.error("ERROR: Player (change_song): Song doesn't have table_index. This should never happen");
				}
				this.playing_index = song.table_index;
			}

			//scroll to song
			var songs_table = this.playlist.songs_table;
			var song_row = songs_table.table.rows[song.table_index];
			var songs_table_scroll = songs_table.table_scrollbar;
	
			songs_table_scroll.scrollTop = song_row.offsetTop - songs_table_scroll.clientHeight/4;
			
			//Highlight Song
			songs_table.select_row(song.table_index);
		}else{
			this.playing_index = this.playlist.get_playlist_index(song.song_id);
		}
		
	};

	//Setup Buttons
	this.play_button = document.createElement("img");
	this.play_button.id = "play_pause";
	this.play_button.src = "svg/play.svg";
	this.play_button.style.display = "block-inline";
	this.play_button.onclick = function (player){
		return function(event){
			//Play first song
			player.audio_obj.play_song(player.playlist.songs[0]);
		};
	}(this);
	this.player_div.appendChild(this.play_button);
	
	
	this.previous_button = document.createElement("img");
	this.previous_button.id = "previous_button";
	this.previous_button.src = "svg/previous.svg";
	this.previous_button.style.display = "block-inline";
	//this.previous_button.style.vertical-align = "middle";
	this.previous_button.onclick = function (player){
		return function(event){
			//play prev song
			//WTF Javascript this should be (player.playing_index - 1)%playlist.songs.length)
			//but this doesn't work with negatives and instead just returns the negative value
			var prev_index = (((player.playing_index - 1)%player.playlist.songs.length)+player.playlist.songs.length)%player.playlist.songs.length;
			var song = player.playlist.songs[prev_index];
			player.audio_obj.play_song(song);
		};
	}(this);
	this.player_div.appendChild(this.previous_button);
	
	this.next_button = document.createElement("img");
	this.next_button.id = "next_button";
	this.next_button.src = "svg/next.svg";
	this.next_button.style.display = "block-inline";
	this.next_button.onclick = function (player){
		return function(event){
			//play next song
			var next_index = (player.playing_index + 1)%(player.playlist.songs.length);
			var next_song = player.playlist.songs[next_index];
			player.audio_obj.play_song(next_song);
		};
	}(this);
	this.player_div.appendChild(this.next_button);
	
	this.shuffle = document.createElement("img");
	this.shuffle.id = "shuffle_button";
	this.shuffle.src = "svg/shuffle.svg";
	this.shuffle.style.display = "block-inline";
	this.shuffle.onclick = function (player){
		return function(event){
			//get current song
			var song = player.playlist.songs[player.playing_index];
			
			//shuffle playlist
			player.playlist.shuffle();
			player.shuffled = 1;
		};
	}(this);
	
	this.player_div.appendChild(this.shuffle);
	
	
	
	
	//Song info
	this.song_info_div = document.createElement("div");
	this.song_info_div.id = "song_info";
	this.song_info_div.style.display = "inline-block";
	this.player_div.appendChild(this.song_info_div);


	//	Status boxes
	this.player_status = document.createElement("div");
	this.player_status.id = "player_status";
	this.player_status.style.display = "inline-block";
	//Time in song
	this.time_elem = document.createElement("span");
	this.player_status.appendChild(this.time_elem);
	//Buffering status
	this.buffer_elem = document.createElement("span");
	this.player_status.appendChild(this.buffer_elem);
	//Shuffled
	this.shuffled_elem = document.createElement("span");
	this.player_status.appendChild(this.shuffled_elem);
	
	this.player_div.appendChild(this.player_status);
	
	//Decoding status div
	this.decoding_status_div = document.createElement("div");
	this.decoding_status_div.id = "decoding_status";
	this.decoding_status_div.style.overflowY = "scroll";
	this.decoding_status_div.style.display = "inline-block";
	
	this.player_div.appendChild(this.decoding_status_div);
	
	this.theme_change_div = document.createElement("div");
	this.theme_change_div.id = "theme_change";
	this.theme_change_div.style.display = "inline-block";
	this.theme_change_div.innerHTML = "Dark/Lite";
	this.theme_change_div.onclick = function(){
		var stylesheet = document.getElementById("stylesheet");
		if(stylesheet.href === stylesheet.baseURI + "style_white.css"){
			stylesheet.href = "/style_black.css";
		}else{
			stylesheet.href = "/style_white.css";
		}
	};
	
	this.player_div.appendChild(this.theme_change_div);
	
	
	this.div = this.player_div;
	
	this.update_time = function (){
		var current_seconds = ( parseInt(this.audio_obj.audio_ele.currentTime % 60,10) < 10) ? "0" + parseInt(this.audio_obj.audio_ele.currentTime % 60, 10):parseInt(this.audio_obj.audio_ele.currentTime % 60, 10);
		var duration_seconds = ( parseInt(this.audio_obj.audio_ele.duration % 60,10) < 10) ? "0" + parseInt(this.audio_obj.audio_ele.duration % 60, 10):parseInt(this.audio_obj.audio_ele.duration % 60, 10);
		
		this.time_elem.innerHTML = parseInt(this.audio_obj.audio_ele.currentTime / 60,10) + ":" +  current_seconds  + "/" + parseInt(this.audio_obj.audio_ele.duration /60, 10) + ":" + duration_seconds;	
	};

	
}