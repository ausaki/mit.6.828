# Lab 0

环境初始化遇到了一些问题.

qemu 编译错误, 原因是我的 gcc 版本比较高, 以及系统中的一个头文件发生了变化. 通过修改 qemu 的代码解决了.

来源: https://github.com/mit-pdos/6.828-qemu/pull/2/files

```diff
diff --git a/configure b/configure
index 6969f6f44..83d0bcfac 100755
--- a/configure
+++ b/configure
@@ -4158,6 +4158,20 @@ if compile_prog "" "" ; then
     getauxval=yes
 fi
 
+##########################################
+# check for sysmacros.h
+
+have_sysmacros=no
+cat > $TMPC << EOF
+#include <sys/sysmacros.h>
+int main(void) {
+    return makedev(0, 0);
+}
+EOF
+if compile_prog "" "" ; then
+    have_sysmacros=yes
+fi
+
 ##########################################
 # End of CC checks
 # After here, no more $cc or $ld runs
@@ -4956,6 +4970,10 @@ if test "$rdma" = "yes" ; then
   echo "CONFIG_RDMA=y" >> $config_host_mak
 fi
 
+if test "$have_sysmacros" = "yes" ; then
+  echo "CONFIG_SYSMACROS=y" >> $config_host_mak
+fi
+
 # Hold two types of flag:
 #   CONFIG_THREAD_SETNAME_BYTHREAD  - we've got a way of setting the name on
 #                                     a thread we have a handle to
diff --git a/hw/9pfs/virtio-9p.c b/hw/9pfs/virtio-9p.c
index 4964da0d7..8ddffad25 100644
--- a/hw/9pfs/virtio-9p.c
+++ b/hw/9pfs/virtio-9p.c
@@ -20,7 +20,9 @@
 #include "virtio-9p-coth.h"
 #include "trace.h"
 #include "migration/migration.h"
-
+#ifdef CONFIG_SYSMACROS
+#include <sys/sysmacros.h>
+#endif
 int open_fd_hw;
 int total_open_fd;
 static int open_fd_rc;
diff --git a/qga/commands-posix.c b/qga/commands-posix.c
index ba8de6243..7e845077c 100644
--- a/qga/commands-posix.c
+++ b/qga/commands-posix.c
@@ -29,6 +29,10 @@
 #include "qemu/queue.h"
 #include "qemu/host-utils.h"
 
+#ifdef CONFIG_SYSMACROS
+#include <sys/sysmacros.h>
+#endif
+
 #ifndef CONFIG_HAS_ENVIRON
 #ifdef __APPLE__
 #include <crt_externs.h>
@@ -45,7 +49,6 @@ extern char **environ;
 #include <arpa/inet.h>
 #include <sys/socket.h>
 #include <net/if.h>
-
 #ifdef FIFREEZE
 #define CONFIG_FSFREEZE
 #endif
```