/*
 * id3v2.h 
 *
 *  Created on: July 26th, 2013 
 *      Author: Reyad Attiyat
 *      Copyright 2013 Reyad Attiyat
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  Code borrowed from oggenc part of the vorbis-tools package.
*/

typedef struct id3v2_id_{
	char id[4];
	char** texts;
} id3v2_id_t;


typedef struct id3v2_ids_{
	id3v2_id_t* ids;
	unsigned int num;
} id3v2_ids_t;

