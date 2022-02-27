#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include <vector> 
#include <sys/stat.h>

// READ FILE CONTENTS IN
std::string getFileContents(const char* filename) {
   std::ifstream file(filename);
   std::string contents;
   if (file) {
      std::ostringstream ss;
      ss << file.rdbuf();
      contents = ss.str();
   }
   return contents;
}

// COMPRESSION
template <typename Iterator>
Iterator compress(const std::string &uncompressed, Iterator result) {
  // Build the dictionary, start with 256.
  int dictSize = 256;
  std::map<std::string,int> dictionary;
  for (int i = 0; i < 256; i++)
    dictionary[std::string(1, i)] = i;
 
  std::string w;
  for (std::string::const_iterator it = uncompressed.begin();
       it != uncompressed.end(); ++it) {
    char c = *it;
    std::string wc = w + c;
    if (dictionary.count(wc))
      w = wc;
    else {
      *result++ = dictionary[w];
      // Add wc to the dictionary. Assuming the size is 4096!!!
      if (dictionary.size()<4096)
         dictionary[wc] = dictSize++;
      w = std::string(1, c);
    }
  }
 
  // Output the code for w.
  if (!w.empty())
    *result++ = dictionary[w];
  return result;
}

// DECOMPRESSION
template <typename Iterator>
std::string decompress(Iterator begin, Iterator end) {
  // Build the dictionary.
  int dictSize = 256;
  std::map<int,std::string> dictionary;
  for (int i = 0; i < 256; i++)
    dictionary[i] = std::string(1, i);
 
  std::string w(1, *begin++);
  std::string result = w;
  //std::cout << "\ndecompressed: " << result <<";";
  std::string entry;
  for ( ; begin != end; begin++) {
    int k = *begin;
    if (dictionary.count(k))
      entry = dictionary[k];
    else if (k == dictSize)
      entry = w + w[0];
    else
      throw "Bad compressed k";
 
    result += entry;
    //std::cout << "\ndecompressed: " << result <<";";
    // Add w+entry[0] to the dictionary.
    if (dictionary.size()<4096)
      dictionary[dictSize++] = w + entry[0];
 
    w = entry;
  }
  return result;
}

// convert from integer to binary string
std::string int2BinaryString(int c, int cl) {
      std::string p = ""; //a binary code string with code length = cl
      int code = c;
      while (c>0) {         
		   if (c%2==0)
            p="0"+p;
         else
            p="1"+p;
         c=c>>1;   
      }
      int zeros = cl-p.size();
      if (zeros<0) {
         std::cout << "\nWarning: Overflow. code " << code <<" is too big to be coded by " << cl <<" bits!\n";
         p = p.substr(p.size()-cl);
      }
      else {
         for (int i=0; i<zeros; i++)  //pad 0s to left of the binary code if needed
            p = "0" + p;
      }
      return p;
}

// convert from binary string to integer
int binaryString2Int(std::string p) {
   int code = 0;
   if (p.size()>0) {
      if (p.at(0)=='1') 
         code = 1;
      p = p.substr(1);
      while (p.size()>0) { 
         code = code << 1;
		   if (p.at(0)=='1')
            code++;
         p = p.substr(1);
      }
   }
   return code;
}

int main(int argc, char* argv[]) {
    // get file contents
    std::string filename;
    if (argv[2]) {
        std::string fname(argv[2]);
        std::size_t pos = fname.find(".");
        filename = fname.substr(0, pos);
    }

    if (*argv[1] == 'c') {
        // compress the file
        std::cout << "COMPRESSING...[" << argv[2] << "] ===> [" << filename << ".lzw] \n";
        std::string contents = getFileContents(argv[2]);
        std::vector<int> compressed;
        compress(contents, std::back_inserter(compressed));
        
        int c = 69;
        int bits = 12;
        std::string p = int2BinaryString(c, bits);
        
        std::string bcode= "";
        for (std::vector<int>::iterator it = compressed.begin() ; it != compressed.end(); ++it) {
        
            p = int2BinaryString(*it, bits);
            //std::cout << "c=" << *it <<" : binary string="<<p<<"; back to code=" << binaryString2Int(p)<<"\n";
            bcode+=p;
        }

        //writing to file
        //std::cout << "string 2 save : "<< bcode << "\n";
        std::string fileName = filename + ".lzw";
        std::ofstream myfile;
        myfile.open(fileName.c_str(),  std::ios::binary);
        
        std::string zeros = "00000000";
        if (bcode.size()%8!=0) //make sure the length of the binary string is a multiple of 8
            bcode += zeros.substr(0, 8-bcode.size()%8);
        
        int b; 
        for (int i = 0; i < bcode.size(); i+=8) { 
            b = 1;
            for (int j = 0; j < 8; j++) {
                b = b<<1;
                if (bcode.at(i+j) == '1')
                b+=1;
            }
            char c = (char) (b & 255); //save the string byte by byte
            myfile.write(&c, 1);  
        }
        myfile.close();

    } else if (*argv[1] == 'e') {
        // extract the file
        std::cout << "EXTRACTING...[" << argv[2] << "] ===> [" << filename << "2.txt] \n";
        std::string zeros = "00000000";
        //reading from a file
        std::ifstream myfile2;
        myfile2.open (argv[2],  std::ios::binary);
        
        struct stat filestatus;
        stat(argv[2], &filestatus );
        long fsize = filestatus.st_size; //get the size of the file in bytes
        
        char c2[fsize];
        myfile2.read(c2, fsize);
        
        std::string s = "";
        long count = 0;
        while(count<fsize) {
            unsigned char uc =  (unsigned char) c2[count];
            std::string p = ""; //a binary string
            for (int j=0; j<8 && uc>0; j++) {         
                if (uc%2==0)
                    p="0"+p;
                else
                    p="1"+p;
                uc=uc>>1;   
            }
            p = zeros.substr(0, 8-p.size()) + p; //pad 0s to left if needed
            s+= p; 
            count++;
        } 
        myfile2.close();
        std::vector<int> compressed;
        for (int i = 0; i < s.length(); i += 12) {
            std::string binaryStr = s.substr(i, 12);
            //std::cout << "current binary string: " << binaryStr << std::endl;
            if (binaryString2Int(binaryStr) != 0) {
                compressed.push_back(binaryString2Int(binaryStr));
            }
        }
        std::string result = decompress(compressed.begin(), compressed.end());
        //writing to file
        std::string fileName = filename + "2.txt";
        std::ofstream myfile;
        myfile.open(fileName.c_str(),  std::ios::binary);
        myfile << result;
        myfile.close();
    } else {
      std::cout << "Invalid argument. Terminating program.\n";
      return 1;
    }
}