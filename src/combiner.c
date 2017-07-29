#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <assert.h>




struct __attribute__((__packed__)) FontIndex {
    char name[5];
    uint32_t offset;
};

int main(int argc,char** argv)
{
    if(argc % 2 != 0 || argc < 3) {
        printf("combine resultFileName name1 file1 name2 file2 nameN fileN \n \
    name contains 3 chars & a number(2 chars) ,for example wjy16");
        exit(0);
    }
    int n = (argc - 2) / 2;

    FILE *resultFP = fopen(argv[1],"wb");
    assert(resultFP !=  0);

    struct FontIndex fontIndex[n];

    int offset = n * sizeof(struct FontIndex) + 1;
    char* buffer;
    int length;

    fseek(resultFP,0,SEEK_SET);
    fwrite(&n,sizeof(uint8_t),1,resultFP);

    for(int i=0;i<n;i++)
    {
       
        assert(strlen(argv[2+2*i])==5);
        strcpy(fontIndex[i].name ,argv[2+2*i]);
        fontIndex[i].offset = offset;

        //write index
        printf("%d  %d\n",i*sizeof(struct FontIndex),offset);
        fseek(resultFP,1 + i*sizeof(struct FontIndex),SEEK_SET);
        fwrite(&fontIndex[i],sizeof(struct FontIndex),1,resultFP);

        //write data
        FILE *fp = fopen(argv[3+2*i],"rb");
        assert(fp != 0);
        
        fseek(fp,0,SEEK_END);
        length = ftell(fp);
        
        buffer = malloc(length);
        fseek(fp,0,SEEK_SET);
        fread(buffer,length,1,fp);
        fclose(fp);


        fseek(resultFP,offset,SEEK_SET);
        fwrite(buffer,length,1,resultFP);
        offset += length;
    }
    fclose(resultFP);


    return 0;
}
