/***********************************************/
/*          ANGE BRUNO OLOU                    */
/*          Client version 2024                */
/*      gcc -std=c11 -Wall CL.c -o CL          */
/*          Pour exécuter : ./CL               */
/***********************************************/

#include "CL_def.h"
#include "Proto_CL.h"

int main(int argc, char *argv[])
{
    key_t key;
    int shm_id; int sem_id;
    int pid_lec, pid_red;

printf("\n####  PID du client : %d  ####\n\n", getpid());

/*/////////////////////// Pour la reception des signaux //////////////////////*/
    signal(SIGUSR1, handler_SIGSUR1);
    signal(SIGUSR2, handler_SIGSUR2);
	  //signal(SIGINT, handler_SIGINT);  // ctrl c


/*/////////////////////////// En attente du lancement du serveur  //////////////////////*/
pause();

key = Creer_cle(FichierCle); // generation de la clé
sem_id = Creer_sem(key);     // creation du semaphare

// recuperation la memoire partagee 
shm_id = shmget(key, sizeof(BUF), IPC_CREAT | 0666);
if (shm_id == -1) {
    perror("Erreur recuperation de la memoire partagee");
    exit(EXIT_FAILURE);
}

// Attachement de la memoire partagee 
p_shm_id = (BUF*) shmat(shm_id, 0, 0);
if ( p_shm_id == (BUF *)-1 ) {
    perror("Erreur attachement à la memoire partagee par le fils");
    exit(EXIT_FAILURE);
}

printf("ID memoire partagee : %d\n", shm_id);  // affichage

// initialisation du pipe
if (pipe(pipe_fd) == -1) {
    perror("Erreur lors de la création du pipe");
    exit(EXIT_FAILURE);
}

/* creation du processus lecteur */
pid_lec = fork();
if (pid_lec == -1) {
	perror("Erreur lors de la création du processus lecteur");
	exit(EXIT_FAILURE);
}

if (pid_lec == 0) {
		printf("Creation du fils lecteur \n");
		printf("Debut de reception des données...\n\n");

    close(pipe_fd[0]); // fermeture de l'extremite lecture du pipe

        /* Processus lecteur */
        while (!terminaison) {
          P(sem_id); // Attente du signal du moniteur
          if (terminaison)
            break; // pour eviter d'afficher deux fois la derniere valeur
          int donne = p_shm_id->tampon[p_shm_id->n];  //recupere element tableau
          //printf("Donnée lue : %d\n", data);
          write(pipe_fd[1], &donne, sizeof(donne)); // envoi de element recuperer au rédacteur
        }

        close(pipe_fd[1]); /* Fermer l'extrémité d'écriture */
        exit(0);
} else {
      pid_red = fork();   // creation processus redacteur
        if (pid_red == -1) {
            perror("Erreur lors de la création du processus rédacteur");
            exit(EXIT_FAILURE);
        }

        if (pid_red == 0) { /* Processus rédacteur */
            printf("Creation du fils Redacteur \n");

            close(pipe_fd[1]); /* fermerture de l'extrémité d'écriture du pipe */

            while (!terminaison) {
                int donne;
                if (read(pipe_fd[0], &donne, sizeof(donne)) > 0) { //lire valeur transmis par lecteur
                    ecrire_donnees(donne);  // affichage 
                }
            }

            close(pipe_fd[0]); /* fermer l'extrémité de lecture */
            exit(0);
        }else{
        /* Processus moniteur */
        close(pipe_fd[0]); // Fermertture extremite lecture pipe
        close(pipe_fd[1]); //Fermeture extremite ecritur pipe

		    pause();  //attente d'un signal
		
        while(!terminaison) {			
			    V(sem_id); // liberation du lecteur pour lire les données
			    pause(); // attente d'un signal SIGUSR1 ou SIGUSR2 du serveur
		    }

        // le moniteur envoir envoi un signal pour le tuer le processus lecteur et redacteur
        kill(pid_lec, SIGUSR2);
        kill(pid_red, SIGUSR2);

        /* attente de la fin du lecteur et redacteur (suicide du processus fils)*/
        waitpid(pid_lec, NULL, 0);
        waitpid(pid_red, NULL, 0);
        }

	}
	/* destructionn */
	Detruire_sem(sem_id);
	shmctl(shm_id, IPC_RMID, NULL);

	printf("\nTerminaison du clients.\n");

    return 0;
}

               
/*/////////////////////////////////////////////////////////////////////////*/
       
void handler_SIGSUR1() /* reception SIGUSR 1 */
{
    signal(SIGUSR1,handler_SIGSUR1);
}

void handler_SIGSUR2() /* reception SIGUSR 2 */
{
    signal(SIGUSR2,handler_SIGSUR2);
    terminaison = 1;
}

//void handler_SIGTERM(int i) /* reception SIGINT  pour Ctrl c
/*{
	terminaison = 1;
	signal(i,handler_SIGTERM);
}*/

key_t Creer_cle(char *nom_fichier)
{
  key_t cle;

  if ( (cle = ftok(nom_fichier, CHAR_SHM)) == -1)
    {
      perror("Creer_cle");
      exit(EXIT_FAILURE);
    }
  return cle;
}

////////////////////////////////////////////////////////////

SEMAPHORE Creer_sem(key_t key)
{
  SEMAPHORE sem;
  int r;
  int val_init=0;
  sem = semget(key, 1, IPC_CREAT|0666);
  if (sem < 0)
    {
      perror("Creer_sem : semget");
      exit(EXIT_FAILURE);
    }
  r = semctl(sem,0,SETVAL,val_init);
  if (r <0)
    {
      perror("Creer_sem : semctl");
      exit(EXIT_FAILURE);
    }
  return sem;
}

/* **************************************** */
/* Destruction d'un ensemble de semaphore   */
/*                                          */
/* On utilise donc semctl avec IPC_RMID     */
/* **************************************** */
void Detruire_sem(SEMAPHORE sem)
{
  if (semctl(sem,0,IPC_RMID,0) != 0){
    if (errno == EINVAL) {  // si sémaphore déjà détruit ou n'existe pas
      //ne rien !
    } else{
      perror("Detruire_sem");
      exit(EXIT_FAILURE);
    }
  }
}

/* **************************************** */
/* Modification de la valeur des semaphores */
/*                                          */
/* appartenant a un ensemble de semaphores. */
/* On utilise donc semop                    */
/* **************************************** */
void Changer_sem(SEMAPHORE sem, int val)
{
  struct sembuf sb[1];

  sb[0].sem_num = 0;
  sb[0].sem_op = val;
  sb[0].sem_flg=0;

  while (semop(sem,sb,1) != 0)
  {
    if (errno == EINTR) {
      // si appel interrompu par signal, sors de la boucle
      break;
    } else{
      perror("Changer_sem");
	    exit(EXIT_FAILURE);
    }
  }
}

/* **************************************** */
/* **************************************** */
void P(SEMAPHORE sem)
{
  Changer_sem(sem,-1);
}

/* **************************************** */
/* **************************************** */
void V(SEMAPHORE sem)
{
  Changer_sem(sem,1);
}

void afficher_donnees(BUF *p_shm) {  // pour afficher les données de la memoire partagée par lecteur
  printf("valeur : %d\n\n", p_shm->tampon[p_shm->n]);
}

void ecrire_donnees(int donne) {   //pour afficher la donné avec l'heure par redacteur
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    printf("Reception à %02d:%02d:%02d de la valeur : %d\n", 
           local->tm_hour, local->tm_min, local->tm_sec, donne);
}