//--------------------------------------------------------------------------
//  Process IPTC data and XMP data.
//--------------------------------------------------------------------------
#include "jhead.h"

#ifndef NULL
#define NULL   ((void *) 0)
#endif

// IPTC entry types known to Jhead (there's many more defined)
#define IPTC_RECORD_VERSION         0x00
#define IPTC_SUPLEMENTAL_CATEGORIES 0x14
#define IPTC_KEYWORDS               0x19
#define IPTC_CAPTION                0x78
#define IPTC_AUTHOR                 0x7A
#define IPTC_HEADLINE               0x69
#define IPTC_SPECIAL_INSTRUCTIONS   0x28
#define IPTC_CATEGORY               0x0F
#define IPTC_BYLINE                 0x50
#define IPTC_BYLINE_TITLE           0x55
#define IPTC_CREDIT                 0x6E
#define IPTC_SOURCE                 0x73
#define IPTC_COPYRIGHT_NOTICE       0x74
#define IPTC_OBJECT_NAME            0x05
#define IPTC_CITY                   0x5A
#define IPTC_STATE                  0x5F
#define IPTC_COUNTRY                0x65
#define IPTC_TRANSMISSION_REFERENCE 0x67
#define IPTC_DATE                   0x37
#define IPTC_COPYRIGHT              0x0A
#define IPTC_COUNTRY_CODE           0x64
#define IPTC_REFERENCE_SERVICE      0x2D
#define IPTC_TIME_CREATED           0x3C
#define IPTC_SUB_LOCATION           0x5C
#define IPTC_IMAGE_TYPE             0x82

//--------------------------------------------------------------------------
// This will convert UTF8 string buf to an 8-bit code string asc.
// This will be correct for data which was originally in a 8bit code, 
// for example Latin1, and was then converted to UTF8
// It will output '?' for characters which cannot be represented with 8 bits.
//--------------------------------------------------------------------------
void UTF8_to_Char(unsigned char* buf, unsigned char* asc)
{
	long b=0;
	unsigned char * a = buf;
	int buflen;
	buflen = strlen((char*)buf);
	for (b=0;b<buflen;a++){
		if (!(*a&128))
			// Byte represents an ASCII character. Direct copy will do.
			*asc=*a;
		else if ((*a&192)==128)
			// Byte is the middle of an encoded character. Ignore.
			continue;
		else if ((*a&224)==192)
			// Byte represents the start of an encoded character in the range
			// U+0080 to U+07FF
			{*asc =((*a&31)<<6);a++;
			*asc=(*asc|(*a&63));b++;}
		else if ((*a&240)==224)
			// Byte represents the start of an encoded character in the range
			// U+07FF to U+FFFF
			{*asc=((*a&15)<<12);a++;
			*asc=(*asc |((a[1]&63)<<6));a++;
			*asc=(*asc|(a[2]&63));b++;b++;}
		else if ((*a&248)==240){
			// Byte represents the start of an encoded character beyond the
			// U+FFFF limit of 16-bit integers
			*asc ='?';
		}
		b++;asc++;
	}
}

//--------------------------------------------------------------------------
//  Process and display IPTC marker.
//
//  IPTC block consists of:
//      - Marker:               1 byte      (0xED)
//      - Block length:         2 bytes
//      - IPTC Signature:       14 bytes    ("Photoshop 3.0\0")
//      - 8BIM Signature        4 bytes     ("8BIM")
//      - IPTC Block start      2 bytes     (0x04, 0x04)
//      - IPTC Header length    1 byte
//      - IPTC header           Header is padded to even length, counting the length byte
//      - Length                4 bytes
//      - IPTC Data which consists of a number of entries, each of which has the following format:
//              - Signature     2 bytes     (0x1C02)
//              - Entry type    1 byte      (for defined entry types, see #defines above)
//              - entry length  2 bytes
//              - entry data    'entry length' bytes
//
//--------------------------------------------------------------------------
void show_IPTC (unsigned char* Data, unsigned int itemlen)
{
    const char IptcSig1[] = "Photoshop 3.0";
    const char IptcSig2[] = "8BIM";
    const char IptcSig3[] = {0x04, 0x04};

    unsigned char * pos    = Data + sizeof(short);   // position data pointer after length field
    unsigned char * maxpos = Data+itemlen;
    unsigned char headerLen = 0;
    unsigned char dataLen = 0;

    if (itemlen < 25) goto corrupt;

    // Check IPTC signatures
    if (memcmp(pos, IptcSig1, sizeof(IptcSig1)-1) != 0) goto badsig;
    pos += sizeof(IptcSig1);      // move data pointer to the next field

    if (memcmp(pos, IptcSig2, sizeof(IptcSig2)-1) != 0) goto badsig;
    pos += sizeof(IptcSig2)-1;          // move data pointer to the next field


	while (memcmp(pos, IptcSig3, sizeof(IptcSig3)) != 0) { // loop on valid Photoshop blocks

		pos += sizeof(IptcSig3); // move data pointer to the Header Length
		// Skip header
		headerLen = *pos; // get header length and move data pointer to the next field
		pos += (headerLen & 0xfe) + 2; // move data pointer to the next field (Header is padded to even length, counting the length byte)

		pos += 3; // move data pointer to length, assume only one byte, TODO: use all 4 bytes

		dataLen = *pos++;
		pos += dataLen; // skip data section

		if (memcmp(pos, IptcSig2, sizeof(IptcSig2) - 1) != 0) {
			badsig: if (ShowTags) {
				ErrNonfatal("IPTC type signature mismatch\n", 0, 0);
			}
			return;
		}
		pos += sizeof(IptcSig2) - 1; // move data pointer to the next field
    }

    pos += sizeof(IptcSig3);          // move data pointer to the next field

    if (pos >= maxpos) goto corrupt;

    // IPTC section found

    // Skip header
    headerLen = *pos++;                     // get header length and move data pointer to the next field
    pos += headerLen + 1 - (headerLen % 2); // move data pointer to the next field (Header is padded to even length, counting the length byte)

    if (pos+4 >= maxpos) goto corrupt;

    // Get length (from motorola format)
    //length = (*pos << 24) | (*(pos+1) << 16) | (*(pos+2) << 8) | *(pos+3);

    pos += 4;                    // move data pointer to the next field

    printf("======= IPTC data: =======\n");

    // Now read IPTC data

	// Modified to handle IPTC Envelope Record which can specify character coding UTF-8
    // starting with Photoshop CS5 there exists this record.
    // If it exists, it will precede the records having version 2

    while (pos < (Data + itemlen-5)) {
        short  signature;
        unsigned char   type = 0;
        short  length = 0;
        char * description = NULL;
        int utf8 = 0;;

        if (pos+5 > maxpos) goto corrupt;

        signature = (*pos << 8) + (*(pos+1));
        pos += 2;

		#define IPTC_CODED_CHARACTER_SET 0x5A
		if (signature == 0x1C01){
          	const char IptcSig3[] = "\033%G";
			//we have an envelope record
    	    type = *pos++;
        	//  check if it specifies coded character code set
			if (type != IPTC_CODED_CHARACTER_SET) goto badsig;
			pos += 2;
        	if (memcmp(pos, IptcSig3, sizeof(IptcSig3)-1) != 0) goto badsig;
        	utf8 = TRUE; // handle IPTC Envelope Record which can specify character coding UTF-8
                         // starting with Photoshop CS5 there exists this record.
                         // If it exists, it will precede the records having version 2
			pos += 3;
			signature = (*pos << 8) + (*(pos+1));
			pos += 2;
		};

		if (signature != 0x1C01 && signature != 0x1c02) break;


        type    = *pos++;
        length  = (*pos << 8) + (*(pos+1));
        pos    += 2;                          // Skip tag length
        if (pos+length > maxpos) goto corrupt;
        // Process tag here
        switch (type) {
            case IPTC_RECORD_VERSION:
                printf("Record vers.  : %d\n", (*pos << 8) + (*(pos+1)));
                break;

            case IPTC_SUPLEMENTAL_CATEGORIES:  description = "SuplementalCategories"; break;
            case IPTC_KEYWORDS:                description = "Keywords"; break;
            case IPTC_CAPTION:                 description = "Caption"; break;
            case IPTC_AUTHOR:                  description = "Author"; break;
            case IPTC_HEADLINE:                description = "Headline"; break;
            case IPTC_SPECIAL_INSTRUCTIONS:    description = "Spec. Instr."; break;
            case IPTC_CATEGORY:                description = "Category"; break;
            case IPTC_BYLINE:                  description = "Byline"; break;
            case IPTC_BYLINE_TITLE:            description = "Byline Title"; break;
            case IPTC_CREDIT:                  description = "Credit"; break;
            case IPTC_SOURCE:                  description = "Source"; break;
            case IPTC_COPYRIGHT_NOTICE:        description = "(C)Notice"; break;
            case IPTC_OBJECT_NAME:             description = "Object Name"; break;
            case IPTC_CITY:                    description = "City"; break;
            case IPTC_STATE:                   description = "State"; break;
            case IPTC_COUNTRY:                 description = "Country"; break;
            case IPTC_TRANSMISSION_REFERENCE:  description = "OriginalTransmissionReference"; break;
            case IPTC_DATE:                    description = "DateCreated"; break;
            case IPTC_COPYRIGHT:               description = "(C)Flag"; break;
            case IPTC_REFERENCE_SERVICE:       description = "Country Code"; break;
            case IPTC_COUNTRY_CODE:            description = "Ref. Service"; break;
            case IPTC_TIME_CREATED:            description = "Time Created"; break;
            case IPTC_SUB_LOCATION:            description = "Sub Location"; break;
            case IPTC_IMAGE_TYPE:              description = "Image type"; break;

            default:
                if (ShowTags){
                    printf("Unrecognised IPTC tag: %d\n", type );
                }
            break;
        }
		if (description != NULL) {
            char TempBuf[32];
            unsigned char Temp2Buf[255];
            unsigned char Temp3Buf[255];
            memset(TempBuf, 0, sizeof(TempBuf));
            memset(TempBuf, ' ', 14);
            memcpy(TempBuf, description, strlen(description));
            strcat(TempBuf, ":");
            
            if (utf8) {
                memset(Temp2Buf, 0, sizeof(Temp2Buf));
                memcpy(Temp2Buf, pos, length);
                memset(Temp3Buf, 0, sizeof(Temp3Buf));
                UTF8_to_Char(Temp2Buf,Temp3Buf);
                printf("%s %*.*s\n", TempBuf, strlen((char*)Temp3Buf), strlen((char*)Temp3Buf),(char*)Temp3Buf);
            }else{
                printf("%s %*.*s\n", TempBuf, length, length, pos);
            }
            
            }
        pos += length;
    }
    return;
corrupt:
    ErrNonfatal("Pointer corruption in IPTC\n",0,0);
}



//--------------------------------------------------------------------------
// Dump contents of XMP section
//--------------------------------------------------------------------------
void ShowXmp(Section_t XmpSection)
{
    unsigned char * Data;
    char OutLine[101];
    int OutLineChars;
    int NonBlank;
    unsigned a;
    NonBlank = 0;
    Data = XmpSection.Data;
    OutLineChars = 0;


    for (a=0;a<XmpSection.Size;a++){
        if (Data[a] >= 32 && Data[a] < 128){
            OutLine[OutLineChars++] = Data[a];
            if (Data[a] != ' ') NonBlank |= 1;
        }else{
            if (Data[a] != '\n'){
                OutLine[OutLineChars++] = '?';
            }
        }
        if (Data[a] == '\n' || OutLineChars >= 100){
            OutLine[OutLineChars] = 0;
            if (NonBlank){
                puts(OutLine);
            }
            NonBlank = (NonBlank & 1) << 1;
            OutLineChars = 0;
        }
    }
}

