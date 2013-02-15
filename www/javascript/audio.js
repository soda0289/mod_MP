function update_decoding_status(music_ui_ctx){
		var decdoing_status_list = document.getElementById("decoding_status");
		
		
		if(music_ui_ctx.decoding_job == null){
			alert("No decoding JOB WTF! Server must be borked");
		}
		
		
		
			if(this.decoding_job.transcode_query.result == null){
				return 0;
			}
			var decoding_status = document.getElementById("decoding_" +this.decoding_job.transcode_query.result.input_source_id);
			if(decoding_status == null){
				decoding_status =  document.createElement('span');
				decoding_status.id = "decoding_" +this.decoding_job.transcode_query.result.input_source_id;
				decdoing_status_list.appendChild(decoding_status);
			}
			decoding_status.innerHTML = this.decoding_job.transcode_query.result.status + " Progress: " + this.decoding_job.transcode_query.result.progress;
			if(parseFloat(this.decoding_job.transcode_query.result.progress) < 100.0){
				setTimeout(
				function(dec_job, music_ui_ctx){
					return function(){
						//decode_song(dec_job.song, music_ui_ctx);
						load_query(dec_job.transcode_query, music_ui_ctx)
					}
				}(this.decoding_job, music_ui_ctx),3000);
				
			}else{
				this.decoding_job.song.sources = new Array();
				var source = {
					source_id : this.decoding_job.transcode_query.result.output_source_id,
				 	source_type : this.decoding_job.transcode_query.result.output_type
				};
				this.decoding_job.song.sources.push(source);
				
				if(music_ui_ctx.playing === 0 && this.decoding_job.song.song_id === music_ui_ctx.next_to_play.song_id){
					play_song(this.decoding_job.song, music_ui_ctx);
				}
				
				decdoing_status_list.removeChild(decoding_status);
				music_ui_ctx.decoding_job.splice(music_ui_ctx.decoding_job.indexOf(this.decdoing_job),1);
			}
			
	
}

function decoding_job(transcode_query, song){
	this.song = song;
	this.transcode_query = transcode_query;
	this.status = "";
	this.progress = 0.0;
}


function decode_song(song, music_ui_ctx){
	
	music_ui_ctx.song_loaded = false;
	
	transcode_query = new music_query(music_ui_ctx.domain, 0, "transcode", null, 0, 0, song.song_id,update_decoding_status);
	var dec_job = new decoding_job(transcode_query, song);
	
	//Add to decdoing_job array
	music_ui_ctx.decoding_job.push(dec_job);
	transcode_query.decoding_job = dec_job;
	
	load_query(transcode_query, music_ui_ctx);
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
		var sources_query = new music_query(music_ui_ctx.domain, 0, "sources", null, 0, 0, song.song_id,sources_query_loaded);
		sources_query.song = song;
		sources_query.source_type = "ogg";
	
		load_query(sources_query, music_ui_ctx);
		
		return 0;
	}else{
		return song.sources.length;
	}

}

function play_song(song, music_ui_ctx){
	if(typeof(song) !== "object"){
		alert("passed play_song undefined song");
		return 0;
	}
	//unhighlight current song
	unhighlight_song(music_ui_ctx);
	//stop current song
	music_ui_ctx.playing = 0;
	
	music_ui_ctx.next_to_play = song;
	if(find_playable_source(song, music_ui_ctx) === 0){
		//Abort playing
		music_ui_ctx.playing = 0;
		return 0;
	}

	music_ui_ctx.playing = 1;
	
	music_ui_ctx.audio_ele.src = "http://"+music_ui_ctx.domain+"/music/play/source_id/" + song.sources[0].source_id;
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