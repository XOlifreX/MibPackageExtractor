#include <iostream>
#include <fstream>
#include <vector>
#include <zlib.h>


struct MIBFileStructure {
	std::string name; // 24 bytes
	int startAddress;
	unsigned long size;
	int headerSize;
	char* content;
};

// is the first byte of this int variable equal to one, then it is little Endian
bool isBigEndian() {
	int temp = 1;
	return !((char*)&temp)[0];
}

// char buffer to int conversion
int convertToInt(const char* bugger, size_t len) {

	int temp = 0;

	if (isBigEndian()) {
		for (int i = 0; i < len; i++)
			((char*)&temp)[3 - i] = bugger[i];
	}
	else
	{
		for (int i = 0; i < len; i++)
			((char*)&temp)[i] = bugger[i];
	}

	return temp;

}

// DDS File Extractor
int main() {

	long bytesRead = 0;
	char WORDBuffer[4];
	std::ifstream stream("../MIBExtractor/bg_stage_misc_pl01.mib", std::ios::binary | std::ios::in);

	// Signature check
	stream.read(WORDBuffer, 4);
	bytesRead += 4;

	if (strncmp(WORDBuffer, "MIB ", 4) != 0) {
		std::cout << "Wrong file type." << std::endl;
		std::cin.get();

		stream.close();

		return 1;
	}

	// Unknown
	stream.read(WORDBuffer, 4);
	stream.read(WORDBuffer, 4);
	stream.read(WORDBuffer, 2);
	bytesRead += 10;

	stream.read(WORDBuffer, 2);
	bytesRead += 2;
	int numOfFiles = convertToInt(WORDBuffer, 2);

	std::vector<MIBFileStructure> files;
	for (int i = 0; i < numOfFiles; i++) {
		stream.read(WORDBuffer, 4);
		bytesRead += 4;

		MIBFileStructure temp = {
			"",
			convertToInt(WORDBuffer, 4),
			0,
			0,
			NULL
		};

		files.push_back(temp);
	}

	// Memory poiner has to point to a spot % 0x10 == 0 until first file header starts.
	while (bytesRead < files[0].startAddress) {
		stream.read(WORDBuffer, 1),
		bytesRead++;
	}

	for (int i = 0; i < numOfFiles; i++) {

		// Always 0
		stream.read(WORDBuffer, 4);
		bytesRead += 4;

		// file size
		stream.read(WORDBuffer, 4);
		bytesRead += 4;
		files[i].size = convertToInt(WORDBuffer, 4);

		// header size
		stream.read(WORDBuffer, 4);
		bytesRead += 4;
		files[i].headerSize = convertToInt(WORDBuffer, 4);

		// Pixelformat info
		stream.read(WORDBuffer, 4);
		bytesRead += 4;
		int pixel1 = convertToInt(WORDBuffer, 4);

		stream.read(WORDBuffer, 4);
		bytesRead += 4;
		int pixel2 = convertToInt(WORDBuffer, 4);

		// Height
		stream.read(WORDBuffer, 2);
		bytesRead += 2;
		int height = convertToInt(WORDBuffer, 2);

		// Width
		stream.read(WORDBuffer, 2);
		bytesRead += 2;
		int width = convertToInt(WORDBuffer, 2);

		// Unknown
		stream.read(WORDBuffer, 4);
		bytesRead += 4;

		// Always 0
		stream.read(WORDBuffer, 4);
		bytesRead += 4;

		// file size repitition
		stream.read(WORDBuffer, 4);
		bytesRead += 4;

		// is compressed (No = 00 00 01 00, Yes = 00 00 02 0)
		stream.read(WORDBuffer, 4);
		bytesRead += 4;
		int isCompressed = convertToInt(WORDBuffer, 4);


		// file name
		int length = (files[i].startAddress + files[i].headerSize) - bytesRead;
		std::string fileName = "";

		for (int j = 0; j < length; j++) {
			stream.read(WORDBuffer, 1);
			bytesRead++;

			fileName += WORDBuffer[0];
		}

		fileName.erase(std::find(fileName.begin(), fileName.end(), '\0'), fileName.end());
		files[i].name = fileName;

		unsigned long fileContentLength = files[i].size * 2;
		char* fileContent = new char[fileContentLength];
		stream.read(fileContent, files[i].size);
		bytesRead += files[i].size;
		int compressResult = 0;

		if (isCompressed == 0x20000)
			compressResult = uncompress((unsigned char*)files[i].content, &fileContentLength, (unsigned char*)fileContent, files[i].size);
		else
			files[i].content = fileContent;

		// DDS File Constructor
		{	
			int bytesWritten = 0;
			auto output = std::fstream("../MIBExtractor/files/extracted/" + files[i].name + ".dds", std::ios::out | std::ios::binary);
			char temp = 0x44;
			// Signature
			output.write((char*)&temp, sizeof(char));
			output.write((char*)&temp, sizeof(char));
			temp = 0x53;
			output.write((char*)&temp, sizeof(char));
			temp = 0x20;
			output.write((char*)&temp, sizeof(char));
			bytesWritten += 4;

			// Header size (always 124)
			temp = 0x7C;
			output.write((char*)&temp, sizeof(char));
			temp = 0;
			output.write((char*)&temp, sizeof(char));
			output.write((char*)&temp, sizeof(char));
			output.write((char*)&temp, sizeof(char));
			bytesWritten += 4;

			// flags (what has valid data)
			temp = 0x07;
			output.write((char*)&temp, sizeof(char));
			temp = 0x10;
			output.write((char*)&temp, sizeof(char));
			temp = 0x08;
			output.write((char*)&temp, sizeof(char));
			temp = 0;
			output.write((char*)&temp, sizeof(char));
			bytesWritten += 4;

			// Width
			char width1 = 0xFF & (width >> 8 * 1);
			char width2 = 0xFF & width;
			output.write((char*)(&width2), sizeof(char));
			output.write((char*)(&width1), sizeof(char));
			temp = 0;
			output.write((char*)&temp, sizeof(char));
			output.write((char*)&temp, sizeof(char));
			bytesWritten += 4;

			// Height
			char height1 = 0xFF & (height >> 8 * 1);
			char height2 = 0xFF & height;
			output.write((char*)(&height2), sizeof(char));
			output.write((char*)(&height1), sizeof(char));
			temp = 0;
			output.write((char*)&temp, sizeof(char));
			output.write((char*)&temp, sizeof(char));
			bytesWritten += 4;

			// dwPitchOrLinearSize (temporarily set to 0)
			output.write((char*)&temp, sizeof(char));
			output.write((char*)&temp, sizeof(char));
			output.write((char*)&temp, sizeof(char));
			output.write((char*)&temp, sizeof(char));
			bytesWritten += 4;

			// Depth (always 0)
			output.write((char*)&temp, sizeof(char));
			output.write((char*)&temp, sizeof(char));
			output.write((char*)&temp, sizeof(char));
			output.write((char*)&temp, sizeof(char));
			bytesWritten += 4;

			// MipMapCount (always 0)
			output.write((char*)&temp, sizeof(char));
			output.write((char*)&temp, sizeof(char));
			output.write((char*)&temp, sizeof(char));
			output.write((char*)&temp, sizeof(char));
			bytesWritten += 4;

			// reserved (11 bytes, always 0)
			for(int j = 0; j < 44; j++)
				output.write((char*)&temp, sizeof(char));
			bytesWritten += 11;

			// size
			temp = 0x20;
			output.write((char*)&temp, sizeof(char));
			temp = 0;
			output.write((char*)&temp, sizeof(char));
			output.write((char*)&temp, sizeof(char));
			output.write((char*)&temp, sizeof(char));
			bytesWritten += 4;

			// Flags
			temp = 0x04;
			output.write((char*)&temp, sizeof(char));
			temp = 0;
			output.write((char*)&temp, sizeof(char));
			output.write((char*)&temp, sizeof(char));
			output.write((char*)&temp, sizeof(char));
			bytesWritten += 4;

			// FourCC (always these 4 bytes)
			temp = 0x44;
			output.write((char*)&temp, sizeof(char));
			temp = 0x58;
			output.write((char*)&temp, sizeof(char));
			temp = 0x54;
			output.write((char*)&temp, sizeof(char));
			temp = pixel1 == 0x10001 && pixel2 == 0x8 ? 0x31 : 0x35;
			output.write((char*)&temp, sizeof(char));
			bytesWritten += 4;

			// unused PIXELFORMAT bytes
			temp = 0;
			for (int j = 0; j < 20; j++)
				output.write((char*)&temp, sizeof(char));
			bytesWritten += 20;

			// Caps
			output.write((char*)&temp, sizeof(char));
			temp = 0x10;
			output.write((char*)&temp, sizeof(char));
			temp = 0;
			output.write((char*)&temp, sizeof(char));
			output.write((char*)&temp, sizeof(char));
			bytesWritten += 4;

			// Caps 2 - 4 & reserved
			for (int j = 0; j < 16; j++)
				output.write((char*)&temp, sizeof(char));
			bytesWritten += 7;

			for (int j = 0; j < files[i].size; j++)
				output.write((char*)(files[i].content + j), sizeof(char));

			output.close();

			delete fileContent;

			std::cout << files[i].name.c_str() << " created successfully." << std::endl;

		}

	}

	std::cout << "File(s) created successfully." << std::endl;

	std::cin.get();

	return 0;

}