#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <vector>

using namespace std;
typedef unsigned char byte;

static std::vector<unsigned char> ReadAllBytes(char const* filename)
{
    ifstream ifs(filename, ios::binary|ios::ate);
    ifstream::pos_type pos = ifs.tellg();

    std::vector<unsigned char>  result(pos);

    ifs.seekg(0, ios::beg);
    ifs.read((char*)result.data(), pos);

    return result;
}

void writeBytes(char const* filename, std::vector<unsigned char>* bytes, int offset = 0, int amount = -1)
{
    if(amount<0)
        amount = (*bytes).size()-offset;
    ofstream outfile(filename, ios::out | ios::binary);
    outfile.write((char*)(*bytes).data()+offset, amount);
    outfile.close();
}

int bytesToInt(std::vector<unsigned char>* bytes, int offset){ //little endian to int
    char buf[4];
    buf[0] = (*bytes)[offset];
    buf[1] = (*bytes)[offset+1];
    buf[2] = (*bytes)[offset+2];
    buf[3] = (*bytes)[offset+3];
    unsigned int* p_int = ( unsigned int * )buf;
    return *p_int;
}

void pushInt(std::vector<unsigned char>* bytes, unsigned int value, int offset = -1){
    unsigned char b[4];
    b[3] = (value >> 24) & 0xFF;
    b[2] = (value >> 16) & 0xFF;
    b[1] = (value >> 8) & 0xFF;
    b[0] = value & 0xFF;
    if(offset<0){
        (*bytes).push_back(b[0]);
        (*bytes).push_back(b[1]);
        (*bytes).push_back(b[2]);
        (*bytes).push_back(b[3]);
    }else{
        (*bytes)[offset] = b[0];
        (*bytes)[offset+1] = b[1];
        (*bytes)[offset+2] = b[2];
        (*bytes)[offset+3] = b[3];
    }
}

void encodeData(std::vector<unsigned char>* source, std::vector<unsigned char>* destination)
{
    int segmentLength = bytesToInt(source, 0); //value of the first word
    int segmentsAmount = bytesToInt(source, 4); //value of second word

    int totalLength = segmentLength*segmentsAmount+8;
    if(totalLength %8 == 4)
        totalLength += 4;

    (*destination).resize(totalLength);
    pushInt(destination,segmentLength,0);
    pushInt(destination,segmentsAmount,4);

    for(int i=0;i<segmentsAmount;i++)
    {
        for(int j=0;j<segmentLength;j++)
        {
            (*destination)[i + j*segmentsAmount + 8] = (*source)[i*segmentLength + j + 8];
        }
    }
}

// source = chunk to decode
// destination = decoded output buffer
void decodeData(std::vector<unsigned char>* source, std::vector<unsigned char>* destination)
{
    int segmentLength = bytesToInt(source, 0); //value of the first word
    int segmentsAmount = bytesToInt(source, 4); //value of second word

    cout << "segmentLength: " << segmentLength << " segmentsAmout: " << segmentsAmount << endl;
    int totalLength = segmentLength*segmentsAmount+8;
    if(totalLength %8 == 4)
        totalLength += 4;

    (*destination).resize(totalLength);

    pushInt(destination,segmentLength,0);
    pushInt(destination,segmentsAmount,4);

    if(segmentsAmount<=0 || segmentLength<=0)
        return;

    byte* bodySource = (*source).data() + 8; //points to the 3rd word in source (v13)
    byte* bodyDestination = (*destination).data() + 8; //points to the 3rd word in destination (v3)
    int segmentCounter = 0;

    do
    {
        int remaining = segmentLength; //remaining bytes to write in this segment
        byte* byteToWrite = (bodySource + segmentCounter); //v9
        do
        {
            *bodyDestination++ = *byteToWrite;
            byteToWrite += segmentsAmount;
            --remaining;
        }
        while(remaining);
        ++segmentCounter;
    }
    while(segmentCounter < segmentsAmount);
}

int main()
{
    //decode:
    std::vector<unsigned char> bytesReaded = ReadAllBytes("data");
    std::vector<unsigned char> bytesToWrite;
    decodeData(&bytesReaded,&bytesToWrite);
    writeBytes("dataDecoded",&bytesReaded);

    //encode:
    /*std::vector<unsigned char> bytesReaded = ReadAllBytes("dataDecoded");
    std::vector<unsigned char> bytesToWrite;
    encodeData(&bytesReaded,&bytesToWrite);
    writeBytes("dataEncoded",&bytesToWrite);*/

    int t; cin >> t;
    return 0;
}
