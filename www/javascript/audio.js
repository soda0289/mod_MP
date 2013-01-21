
function decode_song(song_num, music_ui_ctx){
	var song_id = music_ui_ctx.songs[song_num].song_id;
	var decdoing_status = document.getElementById("decoding_status");
	
	var transcode_query = new music_query("mp.attiyat.net", 0, "transcode", null, 0, 0, song_id,null);

	transcode_query.set_url();
	
	console.log(transcode_query.url);
	load_query(transcode_query, music_ui_ctx);
	if(music_ui_ctx.decoding_job == null){
		alert("WWWWWW");
	}
	
	decoding_status.innerHTML = music_ui_ctx.decoding_job.status + "<br \>Progress: " + music_ui_ctx.decoding_job.progress;
	if(parseFloat(music_ui_ctx.decoding_job.progress) < 100.0){
		setTimeout(function(){decode_song(song_num, music_ui_ctx);},1000);
	}else{
		decoding_status.innerHTML = "";
		music_ui_ctx.loaded_source = music_ui_ctx.decoding_job.output_source_id;
		music_ui_ctx.song_loaded = true;
		play_song(song_num, music_ui_ctx);
	}
}

function find_playable_source(song, music_ui_ctx){
	var sources_query = new music_query("mp.attiyat.net", 0, "sources", null, 0, 0, song.song_id,null);
	
	sources_query.source_type = "ogg";
	
	load_query(sources_query, music_ui_ctx);
	
	if (music_ui_ctx.sources.length > 0){
	
	music_ui_ctx.loaded_source = music_ui_ctx.sources[0].source_id;
	music_ui_ctx.song_loaded = true;
	
	}
	
	return music_ui_ctx.sources.length;
}

function play_song(song_num, music_ui_ctx){
	var song = music_ui_ctx.songs[song_num];

	if(find_playable_source(song, music_ui_ctx) == 0){
		decode_song(song_num, music_ui_ctx);
		return 0;
	}
	

	
	music_ui_ctx.audio_ele.src = "http://mp.attiyat.net/music/play/source_id/" + music_ui_ctx.loaded_source;
	music_ui_ctx.playing_song_id = song.song_id;
	music_ui_ctx.audio_ele.play();
	
	//Change status
	var player_status = document.getElementById("player_status");
	player_status.innerHTML = "Title: " + song.song_title + "<br />Artist: " + song.artist_name + "<br />Album: " + song.album_name;
	
	//Change Play button to pause
	var play_button = document.getElementById("play_pause");
	play_button.src = "svg/pause.svg";
	play_button.onclick = function (music_ui_ctx){
		return function(){
			stop_song(music_ui_ctx);
		};
	}(music_ui_ctx);

	
}

function stop_song(music_ui_ctx){
	var audio_ele = document.createElement("audio");
	//Change Play button back
	play_button = document.getElementById("play_pause");
	play_button.src = "svg/play.svg";
	play_button.onclick = function (music_ui_ctx){
		return function(){
			play_song(0, music_ui_ctx);
		}
	}(music_ui_ctx)
	
	music_ui_ctx.audio_ele.pause();
	
}