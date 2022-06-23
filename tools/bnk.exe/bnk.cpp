#include <filesystem>
#include <vector>

#ifdef _WIN32
#pragma warning(disable:4996)
#endif // _WIN32

int main(int argc, char* argv[])
{
  puts("The original .bnk files got created using Wwise_v2013.2.10. They use SoundBank version number 91.");
  puts("Loading these .bnk files is only possible using the libs from the Wwise_v2013.2.10 SDK.");
  puts("They must be patched to version number to 88 so they can be loaded by the libs from the Wwise_v2013.2.9 SDK.");
  puts("If someone finds the Wwise_v2013.2.10 SDK let me know!\n");

  for (const auto& file : std::filesystem::directory_iterator{ std::filesystem::path(argv[1]) })
  {
    if (file.path().extension().string() != ".bnk")
      continue;

    FILE* inFile = fopen(file.path().string().c_str(), "rb");
    fseek(inFile, 0L, SEEK_END);
    const size_t lSize = ftell(inFile);

    std::vector<unsigned char> fileData(lSize);

    rewind(inFile);
    fread(fileData.data(), lSize, 1, inFile);
    fclose(inFile);
    
    switch (fileData[8])
    {
    case 91:
      {
        fileData[8] = 88;
        FILE* outFile = fopen(file.path().string().c_str(), "wb");
        fwrite(fileData.data(), fileData.size(), 1, outFile);
        fclose(outFile);
      }
      printf("patched: %s\n", file.path().filename().string().c_str());
      break;
    case 88:
      printf("skipped: %s\n", file.path().filename().string().c_str());
      break;
    default:
      printf("unknown version %d: %s\n", fileData[8], file.path().filename().string().c_str());
      break;
    }
  }

  return 0;
}