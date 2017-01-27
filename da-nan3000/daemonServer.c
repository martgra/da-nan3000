#include <arpa/inet.h>
//hello
#include <unistd.h> 
#include <stdlib.h>
#include <stdio.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#define LOKAL_PORT 80
#define BAK_LOGG 10 // Størrelse på for kø ventende forespørsler 
int main ()
{

  struct sockaddr_in  lok_adr;
  struct stat sd_buff;
  int sd, ny_sd;
  int file;
  pid_t process_id =0;
  pid_t sid = 0;
  pid_t fid = 0;
  sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  
  // For at operativsystemet skal reservere porten når tjeneren dør
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

  // Initierer lokal adresse
  lok_adr.sin_family      = AF_INET;
  lok_adr.sin_port        = htons((u_short)LOKAL_PORT); 
  lok_adr.sin_addr.s_addr = htonl(         INADDR_ANY);

  // Kobler sammen socket og lokal adresse
  if ( 0==bind(sd, (struct sockaddr *)&lok_adr, sizeof(lok_adr)) )
  {
      printf("Prosess %d er knyttet til port %d.\n", getpid(), LOKAL_PORT);
      //Child
      process_id = fork();
      if(process_id<0)
	    exit(1);
      //Kill parent
      if(process_id>0)
	    exit(0);
      //session
      sid = setsid();
      if(sid<0)
	    exit(1);
      chdir("/root/webtjener/");
      if(chroot("/root/webtjener/")!=0){
          perror("chroot /root/webtjener");
          return 1;
      }
      close(STDIN_FILENO);
      close(STDOUT_FILENO);
      close(STDERR_FILENO);  

  }
  else
    exit(1);

  // Venter på forespørsel om forbindelse
  listen(sd, BAK_LOGG);
  struct sigaction sigchld_action = {
        .sa_handler = SIG_DFL,
          .sa_flags = SA_NOCLDWAIT
  };
  sigaction(SIGCHLD, &sigchld_action, NULL);
  while(1){ 
    // Aksepterer mottatt forespørsel
    ny_sd = accept(sd, NULL, NULL);
    
   if(0==fork()) {
      file=open("webroot/text/html/index.asis",O_RDONLY);
      fstat(file,&sd_buff);
      dup2(ny_sd, 1); // redirigerer socket til standard utgang
      setuid(1000);
      setgid(1000);
      sendfile(ny_sd,file,0,sd_buff.st_size);
      close(file);

      // Sørger for å stenge socket for skriving og lesing
      // NB! Frigjør ingen plass i fildeskriptortabellen
      shutdown(ny_sd, SHUT_RDWR);
    }

    else {
      exit(0);
      close(ny_sd);
    }
  }
  return 0;
}
