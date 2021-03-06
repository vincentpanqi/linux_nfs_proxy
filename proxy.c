#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpc/pmap_prot.h>

#include "rpc_nfsd.h"
#include "rpc_mountd.h"
#include "xdr.h"

static const struct timeval TIMEOUT = { 20, 0 };
static const struct timeval RETRY_TIMEOUT = { 3, 0 };
#define MNT_SENDBUFSIZE (2048U)
#define MNT_RECVBUFSIZE (1024U)

typedef struct {
  char **hostname;
  struct sockaddr_in saddr;
  struct pmap pmap;
} clnt_addr_t;

void proxy_mount(int s)
{
  CLIENT *clnt;
  char *dirname = "/bli";
  dirpath *mnt3arg = &dirname;
  mountres3 mntres;
  mountres3 *mnt3res = &mntres;

  char *hostname, *mounthost = NULL;

  clnt_addr_t mnt_server = {
    .hostname = &mounthost
  };
  struct pmap *mnt_pmap = &mnt_server.pmap;
  memset(mnt_pmap, '\0', sizeof(*mnt_pmap));
  mnt_pmap->pm_prog = MOUNTPROG;
  mnt_pmap->pm_vers = 3;

  clnt_addr_t nfs_server = {
    .hostname = &hostname
  };
  struct pmap *nfs_pmap = &nfs_server.pmap;
  memset(nfs_pmap, '\0', sizeof(*nfs_pmap));
  nfs_pmap->pm_prog = NFS3_PROGRAM;
  nfs_pmap->pm_vers = 3;

  mnt_server.saddr.sin_family = AF_INET;
  mnt_server.saddr.sin_port = htons(41188);
  mnt_server.saddr.sin_addr.s_addr = inet_addr(NFS_REAL_SERVER_IP_ADDRESS);

  {
    struct sockaddr_in sin;

    sin.sin_family = AF_INET;
    sin.sin_port = 0;
    sin.sin_addr.s_addr = INADDR_ANY;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (bind(s, (struct sockaddr *)&sin, sizeof(struct sockaddr)))
      err(1, "bind");

    if (connect(s, (struct sockaddr *)&mnt_server.saddr, sizeof(mnt_server.saddr)))
      err(1, "connect");
  }

  {
  /* clnt = mnt_openclnt(mnt_server, &msock); */
    clnt = clnttcp_create(&mnt_server.saddr, mnt_pmap->pm_prog, mnt_pmap->pm_vers,
			  &s, MNT_SENDBUFSIZE, MNT_RECVBUFSIZE);

  }
  if (!clnt)
    errx(1, "mnt_openclnt");


  {
    int uid = 0, gid = 0;
    clnt->cl_auth = authunix_create("coucou-machine", uid, gid, 1, &gid);
    if (!clnt->cl_auth)
      errx(1, "authunix_create");
  }

  /* NULLify mnt3res so that xdr_array will allocate memory for us */
  memset(mnt3res, '\0', sizeof(*mnt3res));

  enum clnt_stat stat = clnt_call(clnt, MOUNTPROC_MNT,
			 (xdrproc_t) xdr_dirpath, (caddr_t) mnt3arg,
			 (xdrproc_t) xdr_mountres3, (caddr_t) mnt3res,
			 TIMEOUT);
  printf("%s : clnt_call MOUNTPROC_MNT RPC_SUCCESS ? -> %d\n", __func__,
	 stat == RPC_SUCCESS ? 1 : 0);

  auth_destroy(clnt->cl_auth);
  clnt_destroy(clnt);
  close(s);
}

void proxy_()
{
  proxy_mount(999);
}
