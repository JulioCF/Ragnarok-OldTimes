{
	size_t __k;
	for (__k = 0x6A0; __k < 0x700; __k++) {
		packet_db[packet_ver][__k].len = -1;
		packet_db[packet_ver][__k].func = harmony_parse;
	}
}
