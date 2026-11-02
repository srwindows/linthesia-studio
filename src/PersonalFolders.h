#ifndef LINTHESIA_PERSONAL_FOLDERS_H
#define LINTHESIA_PERSONAL_FOLDERS_H
#include <string>
#include <vector>
struct DeletedFolder {
  std::string ticket,origin;
};
class PersonalFolders {
public:
  PersonalFolders();
  const std::string& Root() const {return m_root;}
  const std::string& TrashRoot() const {return m_trash;}
  bool Ready() const {return !m_root.empty()&&!m_trash.empty()&&!m_order.empty();}
  bool Owns(const std::string& path) const;
  bool OwnedChild(const std::string& path) const;
  bool WritableFolder(const std::string& path) const;
  bool Create(const std::string& parent,const std::string& name,
              std::string& created,std::string& error) const;
  // Create a named subfolder in private, non-listed storage. Its visible
  // parent is assigned separately by FolderPlacements (bundled read-only dir).
  bool CreateLinkedFolder(const std::string& name,std::string& created,
                          std::string& error) const;
  // Roll back a newly created but not yet linked empty folder.
  void CancelLinkedFolder(const std::string& created) const;
  bool Rename(const std::string& source,const std::string& newName,
              std::string& result,std::string& error) const;
  bool Move(const std::string& source,const std::string& targetParent,
            std::string& result,std::string& error) const;
  bool Trash(const std::string& path,std::string& ticket,std::string& error) const;
  std::vector<DeletedFolder> Deleted() const;
  bool Restore(const std::string& ticket,std::string& result,
               std::string& error) const;
  std::vector<std::string> ReadOrder(const std::string& dir) const;
  bool SaveOrder(const std::string& dir,const std::vector<std::string>& names,
                 std::string& error) const;
private:
  std::string OrderFile(const std::string& dir) const;
  std::string m_root,m_trash,m_order;
};
#endif
