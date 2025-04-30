#include <algorithm>
#include <iostream>
#include <limits>
#include <vector>
#include <thread>
#include <mutex>

#include "CRC32.hpp"
#include "IO.hpp"

constexpr size_t maxVal = std::numeric_limits<uint32_t>::max();

/// @brief Переписывает последние 4 байта значением value
void replaceLastFourBytes(std::vector<char> &data, uint32_t value)
{
  std::copy_n(reinterpret_cast<const char *>(&value), 4, data.end() - 4);
}

void findCrc32(size_t stride, size_t start, std::vector<char> data,
               size_t dataOffset, size_t bytesCount, uint32_t targetCrc32,
               bool &isFinded, std::vector<char> &resultData, std::mutex &mutex)
{
  for (size_t i = start; i <= maxVal; i += stride)
  {
    if (isFinded)
    {
      return;
    }

    replaceLastFourBytes(data, uint32_t(i));
    auto curCrc32 = crc32(data.data() + dataOffset, bytesCount, ~targetCrc32);
    
    if (curCrc32 == targetCrc32)
    {
      std::lock_guard<std::mutex> lock{mutex};
      isFinded = true;
      resultData = data;
      return;
    }
  }
}

/**
 * @brief Формирует новый вектор с тем же CRC32, добавляя в конец оригинального
 * строку injection и дополнительные 4 байта
 * @details При формировании нового вектора последние 4 байта не несут полезной
 * нагрузки и подбираются таким образом, чтобы CRC32 нового и оригинального
 * вектора совпадали
 * @param original оригинальный вектор
 * @param injection произвольная строка, которая будет добавлена после данных
 * оригинального вектора
 * @return новый вектор
 */
std::vector<char> hack(const std::vector<char> &original, const std::string &injection)
{
  const uint32_t originalCrc32 = crc32(original.data(), original.size());

  std::vector<char> result(original.size() + injection.size() + 4);
  auto it = std::copy(original.begin(), original.end(), result.begin());
  std::copy(injection.begin(), injection.end(), it);

  std::mutex mutex;

  const size_t threadsCount = std::thread::hardware_concurrency();
  std::vector<std::thread> threads;
  threads.reserve(threadsCount);
  
  bool isFinded = false;
  for (size_t i = 0; i < threadsCount; ++i)
  {
    auto lambda = [&]()
    {
      findCrc32(threadsCount, i, result, original.size(), injection.size() + 4, originalCrc32, std::ref(isFinded), std::ref(result), std::ref(mutex));
      
    };

    threads.emplace_back(std::thread{lambda});
  }

  for (size_t i = 0; i < threadsCount; ++i)
  {
    threads[i].join();
  }
  if (isFinded == false)
  {
    throw std::logic_error("Can't hack");
  }

  return result;
}

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    std::cerr << "Call with two args: " << argv[0]
              << " <input file> <output file>\n";
    return 1;
  }

  try
  {
    const std::vector<char> data = readFromFile(argv[1]);
    const std::vector<char> badData = hack(data, "He-he-he");
    std::cout << "out data:" << badData.data() << '\n';
    writeToFile(argv[2], badData);
  }
  catch (std::exception &ex)
  {
    std::cerr << ex.what() << '\n';
    return 2;
  }
  return 0;
}
