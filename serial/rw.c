#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>


/* Pad a full line if Y is out of bounds
   Pad a full page if Z is out of bounds */
void Padding(FILE *fp, const size_t LINE) 
{
	size_t zero = 0;
	for(size_t i=0; i<LINE; i++) {
		fwrite(&zero, sizeof(uint8_t), 1, fp);
	}
}

void CopyData(FILE *fi, FILE *fo, size_t COPY_FROM, const size_t LINE, int edge[2], size_t no_b) 
{
	int a;
	size_t n = LINE;
	uint8_t out[n];
	if(edge!=NULL && n>0) 
		n-=edge[0];
	if(edge!=NULL && edge[0]>0) {	
		fseek(fi, COPY_FROM, SEEK_SET);
		uint8_t fout[edge[0]];
		if(a = fread(&fout[0], sizeof(uint8_t), 1, fi) != 1)
			printf("%d fread Error %d 1\n", no_b, a);
		for(size_t i=1; i<edge[0]; i++)	
			fout[i] = fout[i-1];
		fwrite(&fout[0], sizeof(uint8_t), edge[0], fo);
		
	}
	if(n>0) {
		if(edge!=NULL) {
		COPY_FROM-=edge[1];
		n-=edge[1];	
		}
		fseek(fi, COPY_FROM, SEEK_SET); 
		if(a = fread(&out[0], sizeof(uint8_t), n, fi) != n)
			printf("%d fread Error %d %d\n", no_b, a, n);
		fwrite(&out[0], sizeof(uint8_t), n, fo);
	}
	if(edge!=NULL && edge[1] > 0) {
		fseek(fi, COPY_FROM+n-1, SEEK_SET);
		uint8_t bout[edge[1]];
		if(a = fread(&bout[0], sizeof(uint8_t), 1, fi) != 1)
			printf("%d fread Error %d 1\n", no_b, a);
		for(size_t i=1; i<edge[1]; i++)
			bout[i] = bout[i-1];
		fwrite(&bout[0], sizeof(uint8_t), edge[1], fo);
	}
}

/* Copy data, pad if necessary (scanline) */
void CopyLine(FILE *fpo, FILE *fpi, const size_t COPY_FROM, const size_t LINE, int edge[2])
{
	size_t zero = 0;
	size_t n = LINE;
	n -= edge[0];
	uint8_t tmp[n];
	for(int i=edge[0]; i>0; i--) {
		fwrite(&zero, sizeof(uint8_t), 1, fpo);
	}
	fseek(fpi, +COPY_FROM, SEEK_SET);
	if(fread(&tmp[0], sizeof(uint8_t), n-edge[1], fpi) == -1337) 
		printf("ignore warning\n");
	fwrite(&tmp[0], sizeof(uint8_t), n-edge[1], fpo);
	for(int i=0; i<edge[1]; i++) {
		fwrite(&zero, sizeof(uint8_t), 1, fpo);
	}	
} 

void Write(FILE *fp, size_t out)
{
	fwrite(&out, sizeof(size_t), 1, fp);
}



