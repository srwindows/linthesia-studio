#ifndef LINTHESIA_FOLDER_ORDERING_H
#define LINTHESIA_FOLDER_ORDERING_H
#include <algorithm>
#include <string>
#include <vector>
// Exact two-position exchange. No insert/shift, no file operations.
namespace FolderOrdering {
inline bool SwapPositions(std::vector<std::string>& paths,
                          const std::string& source,
                          const std::string& target) {
  if(source==target)return false;
  auto first=std::find(paths.begin(),paths.end(),source);
  auto second=std::find(paths.begin(),paths.end(),target);
  if(first==paths.end()||second==paths.end())return false;
  std::iter_swap(first,second);
  return true;
}
}
#endif
