use std::collections::HashSet;
use std::ffi::CString;
use std::os::unix::ffi::OsStrExt;
use std::path::{Path, PathBuf};

#[allow(dead_code)]
pub struct File {
    name: String,
    path_buf: PathBuf,
    stat: Option<libc::stat>,
}

#[allow(dead_code)]
impl File {
    pub fn new(dir: &str, name: &str) -> Self {
        let path_buf = if dir.is_empty() || name.starts_with('/') {
            PathBuf::from(name)
        } else {
            Path::new(dir).join(name)
        };

        let stat = lstat_path(&path_buf);
        if stat.is_none() {
            eprintln!(
                "l: {}: {}",
                path_buf.display(),
                std::io::Error::last_os_error()
            );
        }

        File {
            name: name.to_string(),
            path_buf,
            stat,
        }
    }

    pub fn name(&self) -> &str {
        &self.name
    }

    pub fn path(&self) -> &str {
        self.path_buf.to_str().unwrap_or(&self.name)
    }

    pub fn is_stat(&self) -> bool {
        self.stat.is_some()
    }

    fn mode(&self) -> u32 {
        self.stat.map(|s| s.st_mode).unwrap_or(0)
    }

    pub fn is_dir(&self) -> bool {
        (self.mode() & libc::S_IFMT) == libc::S_IFDIR
    }

    pub fn is_link(&self) -> bool {
        (self.mode() & libc::S_IFMT) == libc::S_IFLNK
    }

    pub fn is_exec(&self) -> bool {
        self.mode() & (libc::S_IXUSR | libc::S_IXGRP | libc::S_IXOTH) != 0
    }

    pub fn is_fifo(&self) -> bool {
        (self.mode() & libc::S_IFMT) == libc::S_IFIFO
    }

    pub fn is_sock(&self) -> bool {
        (self.mode() & libc::S_IFMT) == libc::S_IFSOCK
    }

    pub fn is_blockdev(&self) -> bool {
        (self.mode() & libc::S_IFMT) == libc::S_IFBLK
    }

    pub fn is_chardev(&self) -> bool {
        (self.mode() & libc::S_IFMT) == libc::S_IFCHR
    }

    pub fn is_setuid(&self) -> bool {
        self.mode() & libc::S_ISUID != 0
    }

    pub fn is_setgid(&self) -> bool {
        self.mode() & libc::S_ISGID != 0
    }

    pub fn is_sticky(&self) -> bool {
        self.mode() & libc::S_ISVTX != 0
    }

    pub fn is_link_to_dir(&self) -> bool {
        if !self.is_link() {
            return false;
        }
        // stat (not lstat) to follow the symlink
        if let Some(c_path) = to_cstring(&self.path_buf) {
            unsafe {
                let mut st: libc::stat = std::mem::zeroed();
                if libc::stat(c_path.as_ptr(), &mut st) == 0 {
                    return (st.st_mode & libc::S_IFMT) == libc::S_IFDIR;
                }
            }
        }
        false
    }

    pub fn get_blocks(&self, blocksize: u64) -> u64 {
        let blocks_512 = self.stat.map(|s| s.st_blocks as u64).unwrap_or(0);
        // st_blocks is in 512-byte units
        (blocks_512 * 512 + blocksize - 1) / blocksize
    }

    pub fn get_size(&self) -> i64 {
        self.stat.map(|s| s.st_size).unwrap_or(0)
    }

    pub fn get_inode(&self) -> u64 {
        self.stat.map(|s| s.st_ino).unwrap_or(0)
    }

    pub fn get_nlink(&self) -> u64 {
        self.stat.map(|s| s.st_nlink).unwrap_or(0)
    }

    pub fn get_uid(&self) -> u32 {
        self.stat.map(|s| s.st_uid).unwrap_or(0)
    }

    pub fn get_gid(&self) -> u32 {
        self.stat.map(|s| s.st_gid).unwrap_or(0)
    }

    pub fn get_mtime(&self) -> i64 {
        self.stat.map(|s| s.st_mtime).unwrap_or(0)
    }

    pub fn get_atime(&self) -> i64 {
        self.stat.map(|s| s.st_atime).unwrap_or(0)
    }

    pub fn get_ctime(&self) -> i64 {
        self.stat.map(|s| s.st_ctime).unwrap_or(0)
    }

    pub fn get_modes(&self) -> String {
        let mode = self.mode();
        let file_type = match mode & libc::S_IFMT {
            libc::S_IFDIR => 'd',
            libc::S_IFLNK => 'l',
            libc::S_IFBLK => 'b',
            libc::S_IFCHR => 'c',
            libc::S_IFIFO => 'p',
            libc::S_IFSOCK => 's',
            _ => '-',
        };

        let mut s = String::with_capacity(11);
        s.push(file_type);
        s.push(if mode & libc::S_IRUSR != 0 { 'r' } else { '-' });
        s.push(if mode & libc::S_IWUSR != 0 { 'w' } else { '-' });
        s.push(if mode & libc::S_ISUID != 0 {
            if mode & libc::S_IXUSR != 0 { 's' } else { 'S' }
        } else if mode & libc::S_IXUSR != 0 {
            'x'
        } else {
            '-'
        });
        s.push(if mode & libc::S_IRGRP != 0 { 'r' } else { '-' });
        s.push(if mode & libc::S_IWGRP != 0 { 'w' } else { '-' });
        s.push(if mode & libc::S_ISGID != 0 {
            if mode & libc::S_IXGRP != 0 { 's' } else { 'S' }
        } else if mode & libc::S_IXGRP != 0 {
            'x'
        } else {
            '-'
        });
        s.push(if mode & libc::S_IROTH != 0 { 'r' } else { '-' });
        s.push(if mode & libc::S_IWOTH != 0 { 'w' } else { '-' });
        s.push(if mode & libc::S_ISVTX != 0 {
            if mode & libc::S_IXOTH != 0 { 't' } else { 'T' }
        } else if mode & libc::S_IXOTH != 0 {
            'x'
        } else {
            '-'
        });

        s
    }

    pub fn get_perms(&self) -> String {
        if let Some(c_path) = to_cstring(&self.path_buf) {
            let mut perms = String::with_capacity(3);
            unsafe {
                perms.push(if libc::access(c_path.as_ptr(), libc::R_OK) == 0 {
                    'r'
                } else {
                    '-'
                });
                perms.push(if libc::access(c_path.as_ptr(), libc::W_OK) == 0 {
                    'w'
                } else {
                    '-'
                });
                perms.push(if libc::access(c_path.as_ptr(), libc::X_OK) == 0 {
                    'x'
                } else {
                    '-'
                });
            }
            perms
        } else {
            "???".to_string()
        }
    }

    /// Get the immediate symlink target (one level)
    pub fn get_target(&self) -> Option<File> {
        if !self.is_link() {
            return None;
        }
        match std::fs::read_link(&self.path_buf) {
            Ok(target) => {
                let parent = self
                    .path_buf
                    .parent()
                    .map(|p| p.to_string_lossy().to_string())
                    .unwrap_or_default();
                let target_str = target.to_string_lossy().to_string();
                Some(File::new_no_error(&parent, &target_str))
            }
            Err(_) => None,
        }
    }

    /// Get full symlink chain as a list of names
    pub fn get_link_chain(&self) -> Vec<String> {
        let mut chain = Vec::new();
        let mut seen_inodes = HashSet::new();

        if let Some(stat) = self.stat {
            seen_inodes.insert(stat.st_ino);
        }

        let mut current_path = self.path_buf.clone();
        loop {
            let meta = std::fs::symlink_metadata(&current_path);
            let is_link = meta
                .as_ref()
                .map(|m| m.file_type().is_symlink())
                .unwrap_or(false);
            if !is_link {
                break;
            }

            match std::fs::read_link(&current_path) {
                Ok(target) => {
                    let target_display = target.to_string_lossy().to_string();
                    chain.push(target_display);

                    // Resolve relative to parent of current path
                    let resolved = if target.is_absolute() {
                        target
                    } else {
                        current_path
                            .parent()
                            .map(|p| p.join(&target))
                            .unwrap_or(target)
                    };

                    // Check for loop
                    if let Some(stat) = lstat_path(&resolved) {
                        if !seen_inodes.insert(stat.st_ino) {
                            break; // loop detected
                        }
                    }
                    current_path = resolved;
                }
                Err(_) => break,
            }
        }
        chain
    }

    /// Create a File without printing error on stat failure
    fn new_no_error(dir: &str, name: &str) -> Self {
        let path_buf = if dir.is_empty() || name.starts_with('/') {
            PathBuf::from(name)
        } else {
            Path::new(dir).join(name)
        };
        let stat = lstat_path(&path_buf);
        File {
            name: name.to_string(),
            path_buf,
            stat,
        }
    }

    /// Get the stat of the final target (following all symlinks)
    pub fn get_final_target_stat(&self) -> Option<libc::stat> {
        if let Some(c_path) = to_cstring(&self.path_buf) {
            unsafe {
                let mut st: libc::stat = std::mem::zeroed();
                if libc::stat(c_path.as_ptr(), &mut st) == 0 {
                    return Some(st);
                }
            }
        }
        None
    }
}

fn lstat_path(path: &Path) -> Option<libc::stat> {
    let c_path = to_cstring(path)?;
    unsafe {
        let mut st: libc::stat = std::mem::zeroed();
        if libc::lstat(c_path.as_ptr(), &mut st) == 0 {
            Some(st)
        } else {
            None
        }
    }
}

fn to_cstring(path: &Path) -> Option<CString> {
    CString::new(path.as_os_str().as_bytes()).ok()
}
