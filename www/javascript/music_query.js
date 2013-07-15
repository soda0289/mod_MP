function query_parameters(type){
	this.type = type;
	this.num_results = 0;
	this.sort_by = null;
	this.artist_id = [];
	this.album_id = [];
	this.song_id = [];
	this.source_id = [];
	this.album_name = "";
	this.artist_name = "";
	this.song_title = "";
	this.source_type = null;
	this.output_type = null;
	
	this.clone = function(){
		new_obj = new query_parameters(this.type);
		
		for(var attr in this) {
			if(this.hasOwnProperty(attr)){
				new_obj[attr] = this[attr];
			}
		}
		
		return new_obj;
	};
}


//Music query object
function music_query(hostname, parameters,print_results_function){
	this.count = 0;

	this.print_results = print_results_function;
	this.parameters = parameters;
	this.results = [];
	
	this.hostname = hostname;
	
	this.result = null;
	
	this.url = null;
	
	this.reset = function () {
		this.count = 0;
		this.running = 0;
		this.results = [];
	};

	
	
	this.set_url = function (){
		this.upper = (this.count * this.parameters.num_results).toString();
		this.url = "http://" + this.hostname + "/music/" + this.parameters.type;
		//Check if artist_id or album_id is set
	
		if (this.parameters.album_id instanceof Array && this.parameters.album_id.length && this.parameters.album_id.length > 0) {
			this.url += "/album_id/" + this.parameters.album_id;
		}
		if (this.parameters.artist_id instanceof Array && this.parameters.artist_id.length && this.parameters.artist_id.length > 0) {
			this.url += "/artist_id/" + this.parameters.artist_id;
		}
		if (this.parameters.song_id instanceof Array && this.parameters.song_id.length && this.parameters.song_id.length > 0) {
			this.url += "/song_id/" + this.parameters.song_id;
		}
		if (this.parameters.source_id instanceof Array && this.parameters.source_id.length && this.parameters.source_id.length > 0) {
			this.url += "/source_id/" + this.parameters.source_id;
		}
		if (this.parameters.song_title !== null && this.parameters.song_title !== undefined && this.parameters.song_title.length > 0) {
			this.url += "/song_title/" + this.parameters.song_title;
		}
		if (this.parameters.album_name !== null && this.parameters.album_name !== undefined && this.parameters.album_name.length > 0) {
			this.url += "/album_name/" + this.parameters.album_name;
		}
		if (this.parameters.artist_name !== null && this.parameters.artist_name !== undefined && this.parameters.artist_name.length > 0) {
			this.url += "/artist_name/" + this.parameters.artist_name;
		}
		if (this.parameters.sort_by !== null && this.parameters.sort_by !== undefined && this.parameters.sort_by.length > 0) {
			this.url += "/sort_by/" + this.parameters.sort_by;
		}
		if (this.parameters.type === "sources"){
			if(this.parameters.source_type !== null){
				this.url += "/source_type/" + this.parameters.source_type;
			}
		}
		if (this.parameters.type === "transcode"){
			if(this.parameters.output_type !== null){
				this.url += "/output_type/" + this.parameters.output_type;
			}
		}
		if(this.parameters.num_results > 0){
			this.url += "/limit/" + this.parameters.num_results;
			if(this.upper > 0){
				this.url += "/offset/" + this.upper;
			}
		}
	};
	
	this.load = function (){
		
		this.running = 1;

		this.set_url();
		var upper = this.parameters.num_results;
	
		if (window.XMLHttpRequest){// code for IE7+, Firefox, Chrome, Opera, Safari
			this.xmlhttp = new XMLHttpRequest();
		} else {
			//should really error out
			return -1;
		}
	
	
		var lower = this.count * parseInt(upper,10);
		var url = this.url;
		this.xmlhttp.open("GET",url,true); //Async
		
		//Since each xmlhttp request is an array we pass the index of it to the new function
		this.xmlhttp.onreadystatechange=function(music_query){
			//we then must return a function that takes no parameters to satisfy onreadystatechange
			return function(){
				music_query.proccess();
			};
		}(this);
		
		try{
			this.xmlhttp.send();
		}catch(error){
			console.log("Error could send query:" + error);
		}
	};
	
	this.proccess = function (){
		var json_object;
		
		if (this.xmlhttp.readyState === 4 && this.xmlhttp.status === 200){
			try{
				json_object = JSON.parse(String(this.xmlhttp.responseText), null);
			}catch (err){
				console.log("error: " + err + " on request number. URL:" + this.url);
				
				//Re run query
				this.load();
			}
			//Is the query running
			//Did the server list any results
			
			if("Errors" in json_object && json_object.Errors.length > 0){
				for(var x in json_object.Errors){
					var error = json_object.Errors[x];
					if(error.type === 0){
						console.log(error.header + ":" + error.message);
						//return -1;
					}else{
						console.log(error.header + ":" + error.message);
					}
				}
			
			}
			//concat results
			var json_length;
			
			var object_name = this.parameters.type;


			if(this.parameters.type === "transcode"){
				object_name = "decoding_job";
			}
			
			if(object_name in json_object){
				json_length = json_object[object_name].length;
				this.results = this.results.concat(json_object[object_name]);
			}else{
				json_length = 0;
			}
			
			if(this.print_results !== null){
				this.print_results(this.results);
			}

			//If nothing was returned or if the amount returned is less than num expected stop query
			if (json_length === 0 || this.parameters.num_results === 0 || this.parameters.num_results > json_length){
				this.running = 0;
				if(this.onComplete !== undefined){
					this.onComplete();
				}
			}else{
				this.count++;
				this.load();
			}
			
		}
	};

}
