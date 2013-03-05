
//Music query object
function music_query(hostname, num_results, type,print_results_function){
	this.running = 1;
	this.count = 0;
	
	this.results = [];
	this.num_results = num_results;
	
	this.hostname = hostname;
	this.type = type;
	this.sort_by;
	this.artist_id;
	this.album_id ;
	this.song_id;
	this.source_id;
	this.result = null;
	
	this.source_type = "ogg";
	this.url;
	
	this.reset = function () {
		this.count = 0;
		this.running = 1;
		this.results = [];
	}
	
	
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
		if (this.sort_by !== null && this.sort_by !== undefined) {
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

function concat_query_results(music_query, json_object){
	var json_length;
	
	var object_name;


	if(music_query.type === "transcode"){
		object_name = "decoding_job";
	}else{
		object_name = music_query.type;
	}
	
	
	music_query.results = music_query.results.concat(json_object[object_name]);
	json_length = json_object[object_name].length; 
	//If nothing was returned or if the amount returned is less than num expected stop query
	if (json_length === 0 || music_query.num_results === 0 || music_query.num_results > json_length){
		music_query.running = 0;
	}
}

function proccess_query(music_query){
	if (music_query.xmlhttp.readyState == 4 && music_query.xmlhttp.status == 200){
		try{
			var json_object = JSON.parse(String(music_query.xmlhttp.responseText), null);
		}catch (err){
			alert("error: " + err + " on request number. URL:" + music_query.url);
			
			//Re run query
			load_query(music_query);
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
		concat_query_results(music_query, json_object);
		if(music_query.print_results != null){
			music_query.print_results(music_query.results);
		}
		
		if(music_query.running == 1){
			music_query.count++;
			load_query(music_query);
		}
		
	}
}

function load_query(music_query){

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
		music_query.xmlhttp.onreadystatechange=function(music_query){
			//we then must return a function that takes no parameters to satisfy onreadystatechange
			return function(){
				proccess_query(music_query);
			}
		}(music_query);
		
		try{
			music_query.xmlhttp.send();
		}catch(error){
			console.log("Error could send query:" + error);
		}
}