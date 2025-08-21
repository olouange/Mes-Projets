/* Prototpe des handlers */
void handler_SIGSUR1();
void handler_SIGSUR2();
//void handler_SIGINT(int signal);  // pour CTRL-C ne marche pas bien encore


typedef int SEMAPHORE;
SEMAPHORE Creer_sem(key_t key);
void Detruire_sem(SEMAPHORE sem);
void Changer_sem(SEMAPHORE sem, int val);
void P(SEMAPHORE sem);
void V(SEMAPHORE sem);
key_t Creer_cle(char *nom_fichier);

void afficher_donnees(BUF *p_shm);
void ecrire_donnees(int donne);

/* Variable globales  */
int terminaison = 0; /* le flag de terminaison */
BUF *p_shm_id; /* Pointeur vers la mémoire partagée */
int pipe_fd[2];      /* Descripteurs du pipe */
