function table(columns, row_click_cb, col_click_cb){
	//Selected id in the current table
	this.selected_ids = [];
	
	var column_header_height = "33px";
	
	this.columns = columns;
	this.col_click_cb = col_click_cb;
	
	this.table_div = document.createElement('div');
	this.table_div.style.position = "relative";
	this.table_div.style.display = "inline-block";
	this.table_div.style.height = "100%";
	
	this.clear = function() {
		this.selected_ids = [];
		this.table.innerHTML = "";
	};
	
	this.deselect_row = function(index){
		var row = this.table.rows[index];
		if(row.selected === 1){
			var classes;
			classes = row.className.split(" ");
			var i = classes.indexOf("selected");
			if(i !=-1){
				classes.splice(i,1);
				row.className = classes.join(" ");
				row.selected = 0;
				return 0;
			}

		}
		return -1;
	}
	
	this.select_row = function(index){
		var row = this.table.rows[index];
		if(!("selected" in row) || row.selected === 0){
			row.className += " selected";
			row.selected = 1;
			return 0;
		}else{
			this.deselect_row(index);
		}
		
		return -1;
	};
	
	this.invert_table = function (){
		
		
	}
	
	this.create_column_header = function (){
		//Use header
		this.header_table = document.createElement('table');
		this.header_table.cellSpacing = "0";
		
		//Create table header
		var head_row = document.createElement('tr');
		
		head_row.className = "table_header";
		head_row.style.cursor = "pointer";
		
		for(var col in this.columns){
			var new_col;
			
			new_col = document.createElement('th');
			new_col.style.width = (100 / this.columns.length) + "%";
			
			var column_title = document.createElement("div");
			column_title.className = "column_title";
			column_title.innerHTML =  this.columns[col].header;
			
			new_col.appendChild(column_title);
			
			var arrows_div = document.createElement("div");
			arrows_div.className = "sort_arrows";
			
			var up_arrow = document.createElement("div");
			up_arrow.className = "up_arrow";
			
			var down_arrow = document.createElement("div");
			down_arrow.className = "down_arrow";
			
			up_arrow.onclick = function(){
				console.log("kkk");
			}

			arrows_div.appendChild(up_arrow);
			arrows_div.appendChild(down_arrow);
			
			new_col.appendChild(arrows_div);
			
			
			head_row.appendChild(new_col);
			if(this.col_click_cb !== undefined){
				new_col.onclick = this.col_click_cb(col);
			}
		}
		
		this.header_table.appendChild(head_row);
		this.header_table.style.width = "100%";//this.table.offsetWidth;
		this.header_table.style.height = column_header_height;
		
		
		this.table_div.insertBefore(this.header_table,this.table_scrollbar);
	};
	
	this.create_table = function(){
		//Div with scrollbars to scroll table
		this.table_scrollbar = document.createElement('div');
		this.table_scrollbar.addEventListener("scroll",
				function(music_ui_ctx){
					return function(event){
						
					};
					
				}(this),false);
		this.table_scrollbar.style.width = "100%";
		this.table_scrollbar.style.overflowY = "scroll";
		this.table_scrollbar.style.position = "absolute";
		this.table_scrollbar.style.bottom = "0";
		this.table_scrollbar.style.top = column_header_height;
		
		this.table = document.createElement('table');
		this.table.id = "table";
		this.table.style.width = "100%";
		this.table.cellSpacing = "0";

		this.table_scrollbar.appendChild(this.table);
		this.table_div.appendChild(this.table_scrollbar);
		this.create_column_header();
	};
	
	this.create_table();
	
	this.add_rows_cb = function(table_p, table_c,column_list, row_click_cb){
		
		return function(data){
			for(var i = table_c.rows.length; i < data.length; i = table_c.rows.length){
				var data_elem = data[i];
				data_elem.index = i;
				
				var new_row = document.createElement('tr');
				var parity = (i%2 === 0) ? "even" : "odd";
				new_row.className = parity;
				new_row.style.cursor = "pointer";
				
				for(var col in column_list){
					var new_col;
					new_col = document.createElement('td');
					new_col.style.width = (100 / column_list.length) + "%";
					new_col.innerHTML = data_elem[column_list[col].friendly_name];
					new_row.appendChild(new_col);
				}
				if(row_click_cb !== undefined){
					new_row.onclick = row_click_cb(data_elem, table_p);
				}
				
				table_c.appendChild(new_row);
			}
		};
	}(this, this.table,this.columns, row_click_cb);
	
	this.win_resize = function (table_obj){
		return function(){
		table_obj.header_table.style.width = table_obj.table.offsetWidth + "px";
		};
	}(this);
	
	window.addEventListener('resize', this.win_resize);
	

}