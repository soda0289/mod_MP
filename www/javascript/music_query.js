
//Music query object
function music_query(hostname, num_results, type, sort_by,artist_id,album_id, song_id,print_results_function){
	this.running = 1;
	this.count = 0;
	this.num_results = num_results;
	
	this.hostname = hostname;
	this.type = type;
	this.sort_by = sort_by;
	this.artist_id = artist_id;
	this.album_id = album_id;
	this.song_id = song_id;
	this.source_id;
	this.result = null;
	
	this.source_type = "ogg";
	this.url;
	
	
	this.set_url = function (){
		this.upper = (this.count * this.num_results).toString();
		this.url = "http://" + this.hostname + "/music/" + this.type;
		//Check if artist_id or album_id is set
	
		if (this.album_id > 0) {
			this.url += "/album_id/" + this.album_id;
		}
		if (this.artist_id > 0) {
			this.url += "/artist_id/" + this.artist_id;
		}
		if (this.song_id > 0) {
			this.url += "/song_id/" + this.song_id;
		}
		if (this.source_id > 0) {
			this.url += "/source_id/" + this.source_id;
		}
		if (this.sort_by !== null) {
			this.url += "/sort_by/" + this.sort_by;
		}
		if (type === "sources"){
			if(this.source_type !== null){
				this.url += "/source_type/" + this.source_type;
			}
		}
		if (type === "transcode"){
			this.url += "/output_type/" + this.source_type;
		}
		if(num_results > 0){
			this.url += "/limit/" + num_results;
			if(this.upper > 0){
				this.url += "/offset/" + this.upper;
			}
		}
	}

	this.print_results = print_results_function;
}

function concat_query_results(music_query, music_ui_ctx, json_object){
	var json_length;
	switch (music_query.type){
	case "songs":
		music_ui_ctx.songs = music_ui_ctx.songs.concat(json_object.songs);
		json_length = json_object.songs.length; 
		break;
	case "artists":
		music_ui_ctx.artists = music_ui_ctx.artists.concat(json_object.artists);
		json_length = json_object.artists.length;
		break;
	case "albums":
		music_ui_ctx.albums = music_ui_ctx.albums.concat(json_object.albums);
		json_length = json_object.albums.length;
		break;
	case "sources":
		music_ui_ctx.sources = music_ui_ctx.sources.concat(json_object.sources);
		json_length = json_object.sources.length;
		break;
	case "transcode":
		music_query.result = json_object.decoding_job;
		json_length = json_object.decoding_job.length;
		break;
	}
	//If nothing was returned or if the amount returned is less than num expected stop query
	if (json_length === 0 || music_query.num_results === 0 || music_query.num_results > json_length){
		music_query.running = 0;
	}
}

function proccess_query(music_query, music_ui_ctx){
	if (music_query.xmlhttp.readyState == 4 && music_query.xmlhttp.status == 200){
		try{
			var json_object = JSON.parse(String(music_query.xmlhttp.responseText), null);
		}catch (err){
			alert("error: " + err + " on request number. URL:" + music_query.url);
			
			//Re run query
			load_query(music_query, music_ui_ctx);
		}
		//Is the query running
		//Did the server list any results
		
		if(json_object.Errors !== 'undefined' && json_object.Errors.length > 0){
			for(x in json_object.Errors){
				var error = json_object.Errors[x];
				if(error.type == 0){
					alert(error.header + ":" + error.message);
					//return -1;
				}else{
					console.log(error.header + ":" + error.message)
				}
			}
		
		}
		concat_query_results(music_query, music_ui_ctx, json_object);
		if(music_query.print_results != null){
			music_query.print_results(music_ui_ctx);
		}
		
		if(music_query.running == 1){
			music_query.count++;
			load_query(music_query, music_ui_ctx);
		}
		
	}
}

function load_query(music_query, music_ui_ctx){

		music_query.set_url();
		var upper = music_query.num_results;
	
		if (window.XMLHttpRequest){// code for IE7+, Firefox, Chrome, Opera, Safari
			music_query.xmlhttp = new XMLHttpRequest();
		} else {
			//should really error out
		  	return -1;
		}
	
	
		var lower = music_query.count * parseInt(upper,10);
		var url = music_query.url;
		music_query.xmlhttp.open("GET",url,true); //Async
		
		//Since each xmlhttp request is an array we pass the index of it to the new function
		music_query.xmlhttp.onreadystatechange=function(music_query, music_ui_ctx){
			//we then must return a function that takes no parameters to satisfy onreadystatechange
			return function(){
				proccess_query(music_query, music_ui_ctx);
			}
		}(music_query, music_ui_ctx);
		
		try{
			music_query.xmlhttp.send();
		}catch(error){
			console.log("Error could send query:" + error);
		}
}

function load_queries(music_ui_ctx){
	load_query(music_ui_ctx.songs_query, music_ui_ctx);
	load_query(music_ui_ctx.albums_query, music_ui_ctx);
	load_query(music_ui_ctx.artists_query, music_ui_ctx);
}