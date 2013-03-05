function table(columns, row_click_cb){
	
	var column_header_height = "33px";
	
	this.columns = columns;
	
	this.table_div = document.createElement('div');
	this.table_div.style.minHeight = "inherit";
	this.table_div.style.position = "relative";
	
	this.clear = function() {
		this.table.innerHTML = "";
		this.query.reset();
	};
	

	
	this.create_column_header = function (){
		//Use header
		this.header_table = document.createElement('table');
		
		//Create table header
		var head_row = document.createElement('tr');
		
		head_row.id = "table_header";
		head_row.style.backgroundColor= "black";
		head_row.style.color = "orange";
		
		for(col in this.columns){
			var new_col
			new_col = document.createElement('th');
			new_col.style.width = (100 / this.columns.length) + "%";
			new_col.innerHTML =  this.columns[col].header;
			head_row.appendChild(new_col);
			new_col.onclick = function(table_obj, col_fname){
				return function(){
					table_obj.query.sort_by = col_fname;
					table_obj.clear();
					load_query(table_obj.query);
				}
			}(this, this.columns[col].friendly_name);
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
		this.table.id = "songs_table";
		this.table.style.width = "100%";

		this.table_scrollbar.appendChild(this.table);
		this.table_div.appendChild(this.table_scrollbar);
		this.create_column_header();
	};
	
	this.create_table();
	
	this.add_rows_cb = function(table_c,column_list, row_click_cb){
		
		return function(data){
			for(var i = table_c.rows.length; i < data.length; i = table_c.rows.length){
				var new_row = document.createElement('tr');
				new_row.className = (i%2 == 0) ? "even" : "odd";
				var data_elem = data[i]
				for(col in column_list){
					var new_col;
					new_col = document.createElement('td');
					new_col.style.width = (100 / column_list.length) + "%";
					new_col.innerHTML = data_elem[column_list[col].friendly_name];
					new_row.appendChild(new_col);
				}
				if(row_click_cb !== undefined){
					new_row.onclick = row_click_cb(data_elem);
				}
				
				table_c.appendChild(new_row);
			}
		};
	}(this.table,this.columns, row_click_cb);
	
	this.win_resize = function (table_obj){
		return function(){
		table_obj.header_table.style.width = table_obj.table.offsetWidth;
		}
	}(this)
	
	window.addEventListener('resize', this.win_resize);
	

}