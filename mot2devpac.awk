#07-Jul-2014
#awk -F"#|,| *| *#|,#" -f mot2devpac.awk
{
    	#Only apply this thing in lines that have movem
	#(I could have simply regexp'd the whole string ($0) but maybe we'd hit a comment that has "movem"
	if (($1 ~ /movem/)==1 ||($2 ~/movem/) ==1)
	{
		split($0,splitline,"#|,| *| *#|,#");
		#determine if the string has a label and adjust the starting index accordingly
	    	if (splitline[1] ~ /movem/)
			start=1;
		else
			start=2;

		reglist="";
	    	if ((splitline[start+1] ~ /^[0-9]+$/) == 1)
		{
			#pre-decrement
		    	pre=1;
			bitfield=splitline[start+1];
			if (bitfield>=32768)
			{	reglist=reglist "D0/"; bitfield=bitfield-32768; }
			if (bitfield>=16384)
			{	reglist=reglist "D1/"; bitfield=bitfield-16384; }
			if (bitfield>=8192)
			{	reglist=reglist "D2/"; bitfield=bitfield-8192; }
			if (bitfield>=4096)
			{	reglist=reglist "D3/"; bitfield=bitfield-4096; }
			if (bitfield>=2048)
			{	reglist=reglist "D4/"; bitfield=bitfield-2048; }
			if (bitfield>=1024)
			{	reglist=reglist "D5/"; bitfield=bitfield-1024; }
			if (bitfield>=512)
			{	reglist=reglist "D6/"; bitfield=bitfield-512; }
			if (bitfield>=256)
			{	reglist=reglist "D7/"; bitfield=bitfield-256; }
			if (bitfield>=128)
			{	reglist=reglist "A0/"; bitfield=bitfield-128; }
			if (bitfield>=64)
			{	reglist=reglist "A1/"; bitfield=bitfield-64; }
			if (bitfield>=32)
			{	reglist=reglist "A2/"; bitfield=bitfield-32; }
			if (bitfield>=16)
			{	reglist=reglist "A3/"; bitfield=bitfield-16; }
			if (bitfield>=8)
			{	reglist=reglist "A4/"; bitfield=bitfield-8; }
			if (bitfield>=4)
			{	reglist=reglist "A5/"; bitfield=bitfield-4; }
			if (bitfield>=2)
			{	reglist=reglist "A6/"; bitfield=bitfield-2; }
			if (bitfield>0)
			{	reglist=reglist "A7/"; }

		}
		else
		{
			#post-increment
			pre=0;
			bitfield=splitline[start+2];
			if (bitfield>=32768)
			{	reglist=reglist "A7/"; bitfield=bitfield-32768; }
			if (bitfield>=16384)
			{	reglist=reglist "A6/"; bitfield=bitfield-16384; }
			if (bitfield>=8192)
			{	reglist=reglist "A5/"; bitfield=bitfield-8192; }
			if (bitfield>=4096)
			{	reglist=reglist "A4/"; bitfield=bitfield-4096; }
			if (bitfield>=2048)
			{	reglist=reglist "A3/"; bitfield=bitfield-2048; }
			if (bitfield>=1024)
			{	reglist=reglist "A2/"; bitfield=bitfield-1024; }
			if (bitfield>=512)
			{	reglist=reglist "A1/"; bitfield=bitfield-512; }
			if (bitfield>=256)
			{	reglist=reglist "A0/"; bitfield=bitfield-256; }
			if (bitfield>=128)
			{	reglist=reglist "D7/"; bitfield=bitfield-128; }
			if (bitfield>=64)
			{	reglist=reglist "D6/"; bitfield=bitfield-64; }
			if (bitfield>=32)
			{	reglist=reglist "D5/"; bitfield=bitfield-32; }
			if (bitfield>=16)
			{	reglist=reglist "D4/"; bitfield=bitfield-16; }
			if (bitfield>=8)
			{	reglist=reglist "D3/"; bitfield=bitfield-8; }
			if (bitfield>=4)
			{	reglist=reglist "D2/"; bitfield=bitfield-4; }
			if (bitfield>=2)
			{	reglist=reglist "D1/"; bitfield=bitfield-2; }
			if (bitfield>0)
			{	reglist=reglist "D0/"; }
		}

		#trim trailing /
		reglist=substr(reglist,1,length(reglist)-1);

		#rebuild string
		if (start==1)
			if (pre==1)
				print " " splitline[1] " " reglist "," splitline[3] " " splitline[4] splitline [5]
			else
				print " " splitline[1] " " splitline[2] "," reglist " " splitline[4] splitline [5]
		else
		    	if (pre==1)
				print splitline[1] " " splitline[2] " " reglist "," splitline[4] " " splitline[5] splitline [6]
			else
				print splitline[1] " " splitline[2] " " splitline[3] "," reglist " " splitline[5] splitline [6]
		
	}
	else
	    print $0
}
