#ifndef LINTHESIA_FOLDER_PLACEMENTS_H
#define LINTHESIA_FOLDER_PLACEMENTS_H
#include <string>
#include <vector>
#include <utility>
#include <map>

// Persisted visual containment for immutable bundled folders and mixed roots.
// Original folder bytes and paths are never changed.
class FolderPlacements {
public:
  FolderPlacements(const std::string& sampleRoot,const std::string& personalRoot);
  bool CanContain(const std::string& source,const std::string& destination,
                  std::string& error) const;
  bool Place(const std::string& source,const std::string& destination,
             std::string& error);
  // Atomic virtual relocation of selected folders; originals stay unchanged.
  bool RelocateBatch(const std::vector<std::string>& folders,
                     const std::string& destination,std::string& error);
  // Restore a nested folder to the unified library root, including bundled
  // examples and folders physically nested under a personal directory.
  bool CanReturnToMain(const std::string& source,std::string& error) const;
  bool ReturnToMain(const std::string& source,std::string& error);
  std::string Parent(const std::string& source) const;
  std::vector<std::string> Children(const std::string& parent) const;
  using PlacementSnapshot=std::vector<std::pair<std::string,std::string>>;
  PlacementSnapshot Snapshot() const {return relations;}
  // Fail closed if a different operation changed the folder graph.
  bool RestoreSnapshot(const PlacementSnapshot& expected,
                       const PlacementSnapshot& desired,std::string& error);
  bool ReferencesWithin(const std::string& directory) const;
  // Ajusta referencias virtuales tras renombrar una carpeta fisica, incluidos sus hijos.
  bool RemapAfterRename(const std::string& oldPath,const std::string& newPath,
                        std::string& error);
  // Nombres visibles de carpetas originales: no cambian los archivos instalados.
  std::string DisplayName(const std::string& path) const;
  bool SetDisplayName(const std::string& path,const std::string& name,
                      std::string& error);
private:
  std::string samples,personal,file;
  std::vector<std::pair<std::string,std::string>> relations;
  std::map<std::string,std::string> aliases;
  bool Allowed(const std::string& folder) const;
  std::string PhysicalParent(const std::string& path) const;
  void Load();
  bool Store(std::string& error) const;
};
#endif
