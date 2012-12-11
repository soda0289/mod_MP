


function on_error(){
	
}
function load_song(song_id, music_ui_ctx){
	var request = new XMLHttpRequest();

	var url = "http://mp/music/play/" + song_id;

	request.open("GET", url, true);
	request.responseType = "arraybuffer";

	  // Decode asynchronously
	  request.onload = function() {
		  music_ui_ctx.audio_ctx.decodeAudioData(request.response, 
				  function (music_ui_ctx){

					var audio_ctx = music_ui_ctx.audio_ctx;
					music_ui_ctx.source = audio_ctx.createBufferSource(); // creates a sound source
					return function(buffer){
						music_ui_ctx.source.buffer = buffer;
						music_ui_ctx.source_loaded = 1;
						play_song(0, music_ui_ctx);
						}
					}(music_ui_ctx),
				  on_error);

	  }
	  request.send();
}


function play_song(song_num, music_ui_ctx){
	
	var player_div = document.getElementById("player");
	var songs = music_ui_ctx.songs;
	var audio_ele = document.createElement("audio");
	
	//Change Play button to pause
	play_button = document.getElementById("play_pause");
	play_button.src = "svg/pause.svg";
	play_button.onclick = function (music_ui_ctx){
		return function(){
			stop_song(music_ui_ctx);
		}
	}(music_ui_ctx)
	
	/*
	//Check if song is loaded
	if (music_ui_ctx.source_loaded == 0){
		load_song(songs[song_num].id, music_ui_ctx);
	}else{
		music_ui_ctx.source.connect(music_ui_ctx.audio_ctx.destination);
		music_ui_ctx.source.noteOn(0);
	}
	*/
	audio_ele.src = "http://mp/music/play/" + songs[song_num].id;
	audio_ele.play();
}

function stop_song(music_ui_ctx){
	//Change Play button back
	play_button = document.getElementById("play_pause");
	play_button.src = "svg/play.svg";
	play_button.onclick = function (music_ui_ctx){
		return function(){
			play_song(0, music_ui_ctx);
		}
	}(music_ui_ctx)
	
	music_ui_ctx.source.noteOff(0);
}