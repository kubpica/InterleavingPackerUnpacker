#include <iostream>
#include <vector>
#include <fstream>
#include <windows.h>
#include <sstream>
#include <algorithm>
#include <string>

namespace patch
{
    template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }
}

typedef unsigned char byte;

#include <string>
#include <sys/stat.h> // stat
#include <errno.h>    // errno, ENOENT, EEXIST
#if defined(_WIN32)
#include <direct.h>   // _mkdir
#endif

bool isDirExist(const std::string& path)
{
#if defined(_WIN32)
    struct _stat info;
    if (_stat(path.c_str(), &info) != 0)
    {
        return false;
    }
    return (info.st_mode & _S_IFDIR) != 0;
#else
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
    {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
#endif
}

bool makePath(const std::string& path)
{
#if defined(_WIN32)
    int ret = _mkdir(path.c_str());
#else
    mode_t mode = 0755;
    int ret = mkdir(path.c_str(), mode);
#endif
    if (ret == 0)
        return true;

    switch (errno)
    {
    case ENOENT:
        // parent didn't exist, try to create it
        {
            int pos = path.find_last_of('/');
            if (pos == std::string::npos)
#if defined(_WIN32)
                pos = path.find_last_of('\\');
            if (pos == std::string::npos)
#endif
                return false;
            if (!makePath( path.substr(0, pos) ))
                return false;
        }
        // now, try to create again
#if defined(_WIN32)
        return 0 == _mkdir(path.c_str());
#else
        return 0 == mkdir(path.c_str(), mode);
#endif

    case EEXIST:
        // done!
        return isDirExist(path);

    default:
        return false;
    }
}

using namespace std;

enum colour { DARKBLUE = 1, DARKGREEN, DARKTEAL, DARKRED, DARKPINK, DARKYELLOW, GRAY, DARKGRAY, BLUE, GREEN, TEAL, RED, PINK, YELLOW, WHITE };

struct setcolour
{
   colour _c;
   HANDLE _console_handle;


       setcolour(colour c, HANDLE console_handle)
           : _c(c), _console_handle(0)
       {
           _console_handle = console_handle;
       }
};

// We could use a template here, making it more generic. Wide streams won't work with this version.
basic_ostream<char> &operator<<(basic_ostream<char> &s, const setcolour &ref)
{
    SetConsoleTextAttribute(ref._console_handle, ref._c);
    return s;
}

struct MyStreamingHelper
{
    MyStreamingHelper(std::ostream& out1,
                      std::ostream& out2) : out1_(out1), out2_(out2) {}
    std::ostream& out1_;
    std::ostream& out2_;
};

bool verbose = false;

template <typename T>
MyStreamingHelper& operator<<(MyStreamingHelper& h, T const& t)
{
   if(verbose){
       h.out1_ << t;
       h.out2_ << t;
   }
   return h;
}

MyStreamingHelper& operator<<(MyStreamingHelper& h, std::ostream&(*f)(std::ostream&))
{
   if(verbose){
       h.out1_ << f;
       h.out2_ << f;
   }
   return h;
}

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

void pushInt(std::vector<unsigned char>* bytes, unsigned int value, int offset = -1){ //int to little endian
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

bool isSegmentStyle(std::vector<unsigned char>* source, int blockSize, int offset = 0){ //isInterleaved(??)
    if(blockSize<=8)
        return false;
    int segmentLength = bytesToInt(source, offset);
    int segmentsAmount = bytesToInt(source, offset+4);

    if(blockSize-(blockSize%8)==segmentLength*segmentsAmount+8)
        return true;
    return false;
}

void encodeData(std::vector<unsigned char>* source, std::vector<unsigned char>* destination) //interleave
{
    int segmentLength = bytesToInt(source, 0); //value of the first word
    int segmentsAmount = bytesToInt(source, 4); //value of second word
    (*destination).resize(segmentLength*segmentsAmount+8);
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
void decodeData(std::vector<unsigned char>* source, std::vector<unsigned char>* destination, int offset = 0) //deinterleave
{
    int segmentLength = bytesToInt(source, offset); //value of the first word
    int segmentsAmount = bytesToInt(source, offset+4); //value of second word

    //cout << "segmentLength: " << segmentLength << " segmentsAmout: " << segmentsAmount << endl;
    (*destination).resize(segmentLength*segmentsAmount+8);

    pushInt(destination,segmentLength,0);
    pushInt(destination,segmentsAmount,4);

    if(segmentsAmount<=0 || segmentLength<=0)
        return;

    byte* bodySource = (*source).data() + offset + 8; //points to the 3rd word in source (v13)
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

ofstream structuretxt;
MyStreamingHelper structure(structuretxt, std::cout);

bool isBlock(std::vector<unsigned char>* bytes, int offset, int subBlocksCount, int blocksize, int minSbToHeader = 0, int minSbToFooter = -1){
    if(minSbToHeader>subBlocksCount)
        return false;
    int temp = subBlocksCount+2;
    temp *= 4;
    if(temp%8==4){
        if(temp>blocksize || bytesToInt(bytes, offset+temp) != 0)
            return false;
        temp += 4;
    }
    int temp2 = bytesToInt(bytes, offset+4);
    if(temp != temp2 || temp2 > blocksize || temp2%4!=0)
        return false;
    for(int i = 1; i<subBlocksCount+1; i++){
        int temp3 = bytesToInt(bytes, offset+4+4*i);
        if(temp2 >= temp3 || temp3 > blocksize || temp3%4!=0)
            return false;
        temp3 = temp2;
    }
    if(subBlocksCount>=minSbToFooter && minSbToFooter>0)
        return true;
    temp = offset+(subBlocksCount+1)*4;
    if(bytesToInt(bytes, temp) != blocksize)
        return false;
    return true;
}

int rawBytes = 0;
int headerBytes = 0;
int printDepth = -1;
bool decode = true;

void unpackBlock(std::vector<unsigned char>* bytes, int offset, int blocksize, string dir = "", int depth = 0){
    int subBlocksCount = bytesToInt(bytes, offset);
    if(depth==0){
        if(depth<=printDepth || printDepth < 0) structure << dir << ": [" << offset << "-" << offset+blocksize-1 << "] (" << blocksize;
        dir += "/";
    }
    if(isBlock(bytes,offset,subBlocksCount,blocksize)){
        if(depth<=printDepth+1 || printDepth < 0) structure << " bytes defining " << subBlocksCount << " sub-blocks";
        if(subBlocksCount==0){
            if(depth<=printDepth+1 || printDepth < 0) structure << " or raw data)" << endl;
            makePath(dir);
            writeBytes((dir+"data").c_str(),bytes,offset,blocksize);
        } else if(depth<=printDepth+1 || printDepth < 0) structure << ")" << endl;
        int firtBlockAddress = bytesToInt(bytes, offset+4);
        if(depth<=printDepth || printDepth < 0){
            for(int j = 0; j<depth+1; j++)
                    structure << "\t";
            structure << "header: [0-" << firtBlockAddress-1 << "] (" << firtBlockAddress << " bytes of header)" << endl;
        }
        headerBytes += firtBlockAddress;
        dir += "subBlock";
        for(int i = 0; i<subBlocksCount; i++){
            int suboff = offset+4+i*4;
            int address = bytesToInt(bytes, suboff);
            int subBlockSize = bytesToInt(bytes, suboff+4)-address;
            if(depth<=printDepth || printDepth < 0){
                for(int j = 0; j<depth+1; j++)
                    structure << "\t";
                structure << "sb" << i << ": [" << address << "-" << address+subBlockSize-1 << "] (" << subBlockSize;
            }
            unpackBlock(bytes,offset+address,subBlockSize,dir+patch::to_string(i)+"/",depth+1);
        }
        int temp = offset+(subBlocksCount+1)*4;
        temp = bytesToInt(bytes, temp);
        if(temp < blocksize){
            if(depth<=printDepth || printDepth < 0){
                for(int j = 0; j<depth+1; j++)
                    structure << "\t";
                structure << "footer: [" << temp << "-" << blocksize-1 << "] (" << blocksize-temp;
            }
            unpackBlock(bytes,temp,blocksize-temp,dir+"Footer/",depth+1);
        }
    }else if(blocksize>0){
        rawBytes += blocksize;
        if(depth<=printDepth+1 || printDepth < 0) {
            structure << " bytes of raw data)";
            /* structure << " //ints:";
            int maxToShow = min(8,blocksize);
            for(int i = 0; i<maxToShow; i+=4){
                structure << " " << bytesToInt(bytes, offset+i);
            }
            if(blocksize>8)
                structure << " (...)";*/
            structure << endl;
        }
        makePath(dir);
        if(decode && isSegmentStyle(bytes,blocksize,offset)){ //deinterleave
            std::vector<unsigned char> decoded;
            decodeData(bytes,&decoded,offset);
            writeBytes((dir+"data").c_str(),&decoded);
        }else writeBytes((dir+"data").c_str(),bytes,offset,blocksize);
    }
}

int packBlock(std::vector<unsigned char>* bytes, string targetFolder = "block", int offset = 0, int depth = 0){
    if(!isDirExist(targetFolder))
        return 0;
    if(depth==0){
        structure << "PACKING: " << targetFolder << " [" << offset << "] (";
    }
    int bytesAdded = 0;
    int subBlocksCount = 0;
    string s = targetFolder + "/subBlock";
    while(isDirExist(s+patch::to_string(subBlocksCount))){
        subBlocksCount++;
    }
    if(subBlocksCount==0){
        std::vector<unsigned char> rawData = ReadAllBytes((targetFolder+"/data").c_str());
        //wyrównanie iloœci bajtów do modulo 8:
        int temp = rawData.size()%8;
        for(int i = 0; i<temp; i++)
            rawData.push_back(0x00);
        //zdekodowanie jeœli zadodowany:
        std::vector<unsigned char> encoded;
        std::vector<unsigned char>* toMerge;
        if(decode && isSegmentStyle(&rawData,rawData.size())){
            encodeData(&rawData,&encoded);
            toMerge = &encoded;
        } else toMerge = &rawData;
        //do³¹czenie do ca³oœci:
        (*bytes).reserve((*bytes).size() + (*toMerge).size()); // preallocate memory
        (*bytes).insert( (*bytes).end(), (*toMerge).begin(), (*toMerge).end() );
        structure << (*toMerge).size() << " bytes of raw data)" << endl;
        return (*toMerge).size();
    }
    structure << subBlocksCount << " sub-blocks)" << endl;
    int bytesToReserve = subBlocksCount+2;
    bytesToReserve *= 4;
    if(bytesToReserve%8==4){
        bytesToReserve += 4;
    }
    bytesAdded += bytesToReserve;
    for(int i = 0; i<bytesToReserve; i++) //(*bytes).resize(offset+bytesToReserve);
        (*bytes).push_back(0x00);
    pushInt(bytes,subBlocksCount,offset);
    pushInt(bytes,bytesToReserve,offset+4);
    for(int j = 0; j<depth+1; j++)
        structure << "\t";
    structure << "header: [0-" << bytesToReserve-1 << "] (" << bytesToReserve << " bytes of header)" << endl;
    for(int i = 0; i<subBlocksCount; i++){
        for(int j = 0; j<depth+1; j++)
            structure << "\t";
        structure << "sb" << i << ": [" << bytesAdded << "] (";
        bytesAdded += packBlock(bytes,s+patch::to_string(i),offset+bytesAdded,depth+1);
        pushInt(bytes,bytesAdded,offset+8+i*4);
    }
    for(int j = 0; j<depth; j++)
            structure << "\t";
    structure << "^ [" << offset << "-" << offset+bytesAdded-1 << "] (" << bytesAdded << " bytes defining " << subBlocksCount << " sub-blocks)" << endl;
    return bytesAdded;
}

inline bool exists(const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}

int main()
{
    int mode = 1; //0=packing 1=unpacking
    string fileName;
    string folderName;

    ifstream config;
    config.open("config.txt", std::fstream::in);
    if( !config.good() ){
        cout << "Config not found. Creating new one." << endl;
        config.close();
        ofstream newconfig;
        newconfig.open("config.txt", std::fstream::trunc | std::fstream::out);
        newconfig << "//FileName: Unpacking: file to unpack; Packing: outcome file name;" << endl;
        newconfig << "FileName=Arena Levels.lsm" << endl;
        newconfig << "//FolderName: Unpacking: outcome folder name; Packing: folder to pack;" << endl;
        newconfig << "FolderName=Arena Levels.lsm Unpacked" << endl;
        newconfig << "//Mode: 0=packing 1=unpacking" << endl;
        newconfig << "Mode=1" << endl;
        newconfig << "Verbose=0" << endl;
        newconfig << "PrintDepth=-1" << endl;
        newconfig << "//set \"Decode\" to 0 to deactivate deconding/encoding" << endl;
        newconfig << "Decode=1" << endl;
        newconfig.close();

        cout << "Choose file to (un)pack in config.txt and restart the program." << endl;
        int t = 0; cin >> t;
        return 0;
    }else{
        string slowo;
        string wiersz;
        //std::stringstream ss;
        while(getline( config, wiersz )){
            //ss.str( std::string() ); ss.clear();
            //ss << wiersz;
            //ss >> slowo; //while(ss >> slowo){
            slowo = wiersz.substr(0,wiersz.find("=")+1);
            //cout << slowo << endl;
                if(slowo == "FileName="){
                    fileName = wiersz.substr(wiersz.find("=")+1);
                }else if(slowo == "FolderName="){
                    folderName = wiersz.substr(wiersz.find("=")+1);
                }else if(slowo == "Mode="){
                    mode = atoi(wiersz.substr(wiersz.find("=")+1).c_str());
                }else if(slowo == "Verbose="){
                    verbose = atoi(wiersz.substr(wiersz.find("=")+1).c_str());;
                }else if(slowo == "PrintDepth="){
                    printDepth = atoi(wiersz.substr(wiersz.find("=")+1).c_str());
                }else if(slowo == "Decode="){
                    decode = atoi(wiersz.substr(wiersz.find("=")+1).c_str());;
                }
                slowo.clear();
                wiersz.clear();
        }
    }
    config.close();

    if(mode){
        //unpacking:
        if(!exists(fileName)){
            cout << fileName + " not found." << endl;
            int t = 0; cin >> t;
            return 0;
        }
        std::vector<unsigned char> fileBytes = ReadAllBytes(fileName.c_str());
        structuretxt.open( fileName.substr(0,fileName.find(".")) + "Struct.txt" );
        if(!verbose){
            structuretxt << "Set \"Verbose=\" in config.txt to 1 to generate the structure." << endl;
        }
        cout << "Unpacking... Wait..." << endl;
        unpackBlock(&fileBytes,0, fileBytes.size(), folderName); //28296240
        if(verbose)
            structure << "Headers: " << headerBytes << "B Real data: " << rawBytes << "B" << endl;
        else
            cout << "Headers: " << headerBytes << "B Real data: " << rawBytes << "B" << endl;
        cout << "Unpacking finished." << endl;
        structure << endl;
    }else{
        //packing:
        std::vector<unsigned char> bytesToWrite;
        cout << "Packing... Wait..." << endl;
        packBlock(&bytesToWrite, folderName);
        writeBytes(fileName.c_str(),&bytesToWrite);
        cout << "Packing finished." << endl;
    }

    int t = 0; cin >> t;
    return 0;
}
