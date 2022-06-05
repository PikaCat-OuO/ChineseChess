#pragma once
#include "featuretransformer.h"

#include "windows.h"
#include <memory>

namespace PikaChess {
/** NNUE文件的哈希值 */
constexpr quint32 HASH_VALUE_FILE = FeatureTransformer::getHashValue() ^ Model::getHashValue();

/** 对齐大页分配 */
inline void *AlignedLargePageAlloc(quint64 allocSize) {
  HANDLE hProcessToken { };
  LUID luid { };
  void* mem = nullptr;

  const quint64 largePageSize = GetLargePageMinimum();
  if (not largePageSize) return nullptr;

  // 提升权限以获得SeLockMemory权限
  if (not OpenProcessToken(GetCurrentProcess(),
                           TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hProcessToken)) return nullptr;

  if (LookupPrivilegeValue(NULL, SE_LOCK_MEMORY_NAME, &luid)) {
    TOKEN_PRIVILEGES tp { };
    TOKEN_PRIVILEGES prevTp { };
    DWORD prevTpLen = 0;

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // 调整令牌权限
    AdjustTokenPrivileges(
        hProcessToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &prevTp, &prevTpLen);

    // 检查是否成功获取权限
    if (GetLastError() == ERROR_SUCCESS) {
      // 向上取整到页的大小，并分配页面
      allocSize = (allocSize + largePageSize - 1) & ~size_t(largePageSize - 1);
      mem = VirtualAlloc(
          NULL, allocSize, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);

      // 恢复原有的令牌
      AdjustTokenPrivileges(hProcessToken, FALSE, &prevTp, 0, NULL, NULL);
    }
  }

  CloseHandle(hProcessToken);

  // 如果分配成功，返回地址，如果分配失败，使用普通API重新分配
  return mem ? mem : VirtualAlloc(NULL, allocSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void NNUEInit();

/** RAII，自动释放内存 */
template <typename T>
struct AlignedDeleter {
  void operator()(T* ptr) const {
    ptr->~T();
    _mm_free(ptr);
  }
};

/** RAII，自动释放内存 */
template <typename T>
struct LargePageDeleter {
  void operator()(T* ptr) const {
    ptr->~T();
    if (ptr and not VirtualFree(ptr, 0, MEM_RELEASE)) {
      DWORD err = GetLastError();
      std::cerr << "无法分配对齐大页， 错误代码: 0x"
                << std::hex << err
                << std::dec << std::endl;
      exit(EXIT_FAILURE);
    }
  }
};

template <typename T>
using AlignedPtr = std::unique_ptr<T, AlignedDeleter<T>>;

template <typename T>
using LargePagePtr = std::unique_ptr<T, LargePageDeleter<T>>;

/** 输入特征转换器，由于参数都在里面，所以使用大页面管理 */
extern LargePagePtr<FeatureTransformer> featureTransformer;

/** 模型剩余的部分使用对齐页面 */
extern AlignedPtr<Model> model[LAYER_STACKS];
}
