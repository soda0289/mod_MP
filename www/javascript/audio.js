function update_decoding_status(music_ui_ctx){
		var decdoing_status = document.getElementById("decoding_status");
		
		if(music_ui_ctx.decoding_job[0] == null){
			alert("No decoding JOB WTF! Server must be borked");
		}
		
		decoding_status.innerHTML = music_ui_ctx.decoding_job[0].status + "<br \>Progress: " + music_ui_ctx.decoding_job[0].progress;
		if(parseFloat(music_ui_ctx.decoding_job[0].progress) < 100.0){
			setTimeout(
			function(transcode_query, music_ui_ctx){
				return function(){
					decode_song(transcode_query.song, music_ui_ctx);
				}
			}(this, music_ui_ctx),3000);
		}else{
			decoding_status.innerHTML = "";
			this.song.sources = new Array();
			var source = {
				source_id : music_ui_ctx.decoding_job[0].output_source_id,
			 	source_type : music_ui_ctx.decoding_job[0].output_type
			};
			this.song.sources.push(source);
			
			if(music_ui_ctx.playing === 0){
				play_song(this.song, music_ui_ctx);
			}
		}
}


function decode_song(song, music_ui_ctx){
	
	music_ui_ctx.song_loaded = false;
	
	music_ui_ctx.transcode_query = new music_query("mp.attiyat.net", 0, "transcode", null, 0, 0, song.song_id,update_decoding_status);
	
	music_ui_ctx.transcode_query.song = song;

	music_ui_ctx.decoding_job = new Array();
	
	load_query(music_ui_ctx.transcode_query, music_ui_ctx);
}

function sources_query_loaded(music_ui_ctx){
	if (music_ui_ctx.sources.length > 0){
		//Copy sources to song
		this.song.sources = music_ui_ctx.sources;
		//clear sources
		music_ui_ctx.sources = new Array();

		if(music_ui_ctx.playing === 0){
			play_song(music_ui_ctx.songs[music_ui_ctx.playing_index], music_ui_ctx);
		}
		
	}else{
		//Decode song no source avalible
		decode_song(this.song, music_ui_ctx);
	}
}

function find_playable_source(song, music_ui_ctx){
	if(!('sources' in song) || song.sources.length < 1){
		//song has no sources
		//Fetch sources
		var sources_query = new music_query("mp.attiyat.net", 0, "sources", null, 0, 0, song.song_id,sources_query_loaded);
		sources_query.song = song;
		sources_query.source_type = "ogg";
	
		load_query(sources_query, music_ui_ctx);
		
		return 0;
	}else{
		return song.sources.length;
	}

}

function play_song(song, music_ui_ctx){
	if(typeof(song) === "undefined"){
		alert("passed play_song undefined song");
		return 0;
	}
	
	if(find_playable_source(song, music_ui_ctx) === 0){
		//Abort playing
		music_ui_ctx.playing = 0;
		return 0;
	}

	music_ui_ctx.playing = 1;
	
	music_ui_ctx.audio_ele.src = "http://mp.attiyat.net/music/play/source_id/" + song.sources[0].source_id;
	music_ui_ctx.audio_ele.load;
	
	//scroll to song
	var song_listing = document.getElementById("song_" + song.song_id);
	song_listing.scrollIntoView();
	var scrollBack = (music_ui_ctx.songs_div.scrollHeight - music_ui_ctx.songs_div.scrollTop <= music_ui_ctx.songs_div.clientHeight) ? 0 : music_ui_ctx.songs_div.clientHeight/4;
	music_ui_ctx.songs_div.scrollTop -= scrollBack;
    //Highlight Song
	music_ui_ctx.song_backgroundColor = song_listing.style.backgroundColor;
	song_listing.style.backgroundColor = "black";
	


	
	
	//Change status
	music_ui_ctx.song_info_elem.innerHTML = "Title: " + song.song_title + "<br />Artist: " + song.artist_name + "<br />Album: " + song.album_name;
	
	//Change Play button to pause
	music_ui_ctx.play_button.src = "svg/pause.svg";
	music_ui_ctx.play_button.onclick = function (music_ui_ctx){
		return function(){
			stop_song(music_ui_ctx);
		};
	}(music_ui_ctx);
	//Play next song when finished current
	music_ui_ctx.audio_ele.addEventListener('ended', function(music_ui_ctx){
			return function(event){
				music_ui_ctx.playing = 0;
				play_song(music_ui_ctx.songs[get_next_song_index(music_ui_ctx)],music_ui_ctx);
			}
		}(music_ui_ctx), false);
	
		for(var i = 1;i <= 4; i++){
			var song_next = music_ui_ctx.songs[music_ui_ctx.playing_index + i];
			if(typeof song_next !== 'undefined'){
				find_playable_source(song_next, music_ui_ctx);
			}else{
				break;
			}
		}
}

function stop_song(music_ui_ctx){
	var audio_ele = document.createElement("audio");
	//Change Play button back
	music_ui_ctx.play_button.src = "svg/play.svg";
	music_ui_ctx.play_button.onclick = function (music_ui_ctx){
		return function(){
			//Change Play button to pause
			music_ui_ctx.audio_ele.play();
			music_ui_ctx.play_button.src = "svg/pause.svg";
			music_ui_ctx.play_button.onclick = function (music_ui_ctx){
				return function(){
					stop_song(music_ui_ctx);
				};
			}(music_ui_ctx);
			
		}
	}(music_ui_ctx)
	
	music_ui_ctx.audio_ele.pause();
	
}