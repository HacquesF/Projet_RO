/* NOM1 Prénom1
   NOM2 Prénom2 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glpk.h> /* Nous allons utiliser la bibliothèque de fonctions de GLPK */

#include <time.h>
#include <sys/time.h>
#include <sys/resource.h> /* Bibliothèques utilisées pour mesurer le temps CPU */

/* structures et fonctions de mesure du temps CPU */

struct timeval start_utime, stop_utime;

void crono_start()
{
	struct rusage rusage;
	
	getrusage(RUSAGE_SELF, &rusage);
	start_utime = rusage.ru_utime;
}

void crono_stop()
{
	struct rusage rusage;
	
	getrusage(RUSAGE_SELF, &rusage);
	stop_utime = rusage.ru_utime;
}

double crono_ms()
{
	return (stop_utime.tv_sec - start_utime.tv_sec) * 1000 +
    (stop_utime.tv_usec - start_utime.tv_usec) / 1000 ;
}

/* Structure contenant les données du problème */

typedef struct {
	int nblieux; /* Nombre de lieux (incluant le dépôt) */
	int capacite; /* Capacité du véhicule de livraison */
	int *demande; /* Demande de chaque lieu (la case 0 est inutilisée car le dépôt n'a aucune demande à voir satisfaire) */
	int **C; /* distancier (les lignes et colonnes 0 correspondent au dépôt) */
} donnees;

/* Structure contenant un trajet */
typedef struct {
  int* chemin;//les points a traverser
  int nbplace;//Le nombre de point non base visite
  int longueur;//La longueur minimale pour faire la boucle
} trajet;
//-------------Nb trajet totale possible Somme de i=1 a nblieux ( n!/(i!(n-i)!))-----------

//Calcul d un factoriel, necessaire au nombre de regroupement max
int facto(int n){
  if (n==0)
    return 1;
  else
    return n*facto(n-1);
}

/* enumeration des regroupements */
int** enumererRegroupe(donnees *p){
  int i,j;
  //On compte le nombre de trajet max
  int totT=0;
  //p->nblieux semble compter le camp de base, or il n est pas dans les regroupement, a voir si ne pas le -1
  int nblieux = p->nblieux -1;
  int factoNblieux= facto(nblieux);//Pour reduire le nombre de calcul
  for(i=1;i<nblieux;++i)
    totT+= factoNblieux/( facto(i) * facto(nblieux-i));
  
  ++ totT;// Le regroupement de nblieux
  //++totT;//Une case supplementaire au cas où on a tous les regroupement pour le 0
  //Il semblerait que le tableau aille jusque [totT] en partant de 0 donc totT+1 case
  int ** lesRegroup;
  lesRegroup=(int **) malloc (totT * sizeof(int *));
  for(i=0;i < totT;++i) lesRegroup[i] = (int *) malloc ( (nblieux+1) * sizeof(int));
  //On a le tableau de dimension 2 pouvant tout contenir
  int taille, actuel, depart, ligne;
  int bLigne, bDepart;//On les considere comme des booleens
  // taille = 1;
  //ligne = 0;
  //Pour  l instant on fait sans enlever les trop plein
  //-----------------------Ca fait de la merde--------------------------------
  //-----------------------Ge--Pa--Teste----------------------------------
  /* TODO
     Mettre un tmp de remplissage pour savoir si on prend le chemin où le balance
     Compter le nombre de regroupement pour ne pas sortir du tableau quand on le relit
   */

  /* while( taille <= p->nblieux ){ */
  /*   bDepart = 1; */
  /*   depart= 1; */
  /*   while(bDepart==1 ){//On a tous les regoupement de taille taille */
  /*     j=2;//On commence a la colonne 2 car 0 pour la taille et 1 pour le depart */
  /*     lesRegroup[ligne][0]= taille; // On instancie la taille */
  /*     lesRegroup[ligne][1]= depart; */
  /*     bLigne = 1; */
  /*     actuel = 1; */
  /*     while( bLigne == 1){//On a tout les regroupements de taille taille commencant a depart */
  /* 	if( actuel > p->nblieux || ligne >= totT){ */
  /* 	  bLigne = 0; */
  /* 	}else if( j> taille ){//Si on a suffisament de chiffre */
  /* 	   ++ligne;//On change de ligne */
  /* 	   lesRegroup[ligne][0]= taille;//On met la taille de la nouvel ligne */
  /* 	   lesRegroup[ligne][1]= depart;//On lui met son point de depart */
  /* 	   j=2;//On se prepare pour la prochaine case */
  /* 	}else{//Cas standard */
  /* 	  lesRegroup[ligne][j]= actuel;//On met un chiffre dans la ligne */
  /* 	  ++j;//On passe a la case suivante */
  /* 	} */
  /* 	++actuel;//On passe au chiffre suivant */
  /*     } */
  /*     depart ++;//On reste de la meme taille mais on augmente le nombre de depart */
  /*     if(depart+taille > p->nblieux+1 ){//On ne pourra pas faire de regroupement commencant par ce nmbre et de cette taille (12345=> depart 1 + taille 5 == nblieux 5 + 1 car on, commence a 1 */
  /*     	bDepart = 0;//On doit sortir pour augmenter la taille */
  /*     } */
  /*   } */
  /*   taille++;//On augmente la taille */
  /* } */
  /* lesRegroup[ligne][0]= 0;//Sert de stop pour ne pas lire trop loin */
  //---------------------------Essaie numero 2------------------------------------
  //On insert la premiere ligne
  taille = 1;
  ligne = 0;
  lesRegroup[ligne][0]=taille;
  lesRegroup[ligne][1]=1;
  /*On va travailler en correction
    Donc on verifie la ligne du dessus pour savoir si on se met apres ou si on l ecrase(ligne++ ou non)
    On utilise la ligne du dessus pour savoir ce que contient la ligne actuel:
                              -Si ne finit pas par nblieux alors
			                   - on rajoute 1 a la derniere case
			                   -sinon la prochaine ligne augmente le nombre le plus loin a droite sans que ca depasse avec la taille (pos+(nblieux-nb))<taille+1 (avec nb le nombre a mettre dans la case pos
				       -Si le sinon arrive a 0 (case reserve a la taille) on augmente la taille et repart de 1 (simple boucle for pour remplir la ligne a 1 2 3 ..)
    Normalement la taille ne depassera pas la taille car le compte semble être bon, sinon sauter le for principal en while taille<nblieux
    
   */
  int lignePrec = 0;
  int breakPoint,valBreak;//regarde a quel endroit on casse la chaine prec pour faire l actuel
  while(taille<nblieux){
    //On verifie que la ligne precedement ecrite ne depasse pas
    actuel = 0;
    for(j=0;j<lesRegroup[ligne][0];++j){
      actuel += p->demande[ lesRegroup[ligne][j+1]];
    }
    lignePrec=ligne;
    if(actuel<=p->capacite){
      ++ligne;
    }
    if(lesRegroup[ lignePrec ][taille] != nblieux){//Si le dernier n'est pas nblieux
      //On suppose que l'on ne sera pas au dessus car ca ne doit pas arriver
      //((ligne==0) ? 0 : (ligne-1)): Cette merde sert a utiliser ligne-1 sans tomber dans le negatif
      //Utilise un prec IDIOT
      lesRegroup[ ligne ][0]=taille;
      for(j=1;j<lesRegroup[lignePrec][0];++j){
	lesRegroup[ligne][j]=lesRegroup[lignePrec][j];
      }
      lesRegroup[ligne][taille]=lesRegroup[lignePrec][taille]+1;
      
    }else{
      //On sait que la derniere case est le nombre max
      breakPoint=taille-1;
      while(breakPoint>0 && lesRegroup[lignePrec][breakPoint]+1+taille-breakPoint>nblieux){
	--breakPoint;
      }
      if(breakPoint>0){
	lesRegroup[ligne][0]=taille;
	for(j=1;j<breakPoint;++j){
	  lesRegroup[ligne][j]=lesRegroup[lignePrec][j];
	}
	valBreak = lesRegroup[lignePrec][breakPoint]+1;
	lesRegroup[ligne][breakPoint]= valBreak;
	for(j=1;j<=taille-breakPoint;++j){
	  lesRegroup[ligne][breakPoint+j]=valBreak+j;
	}
      }else{
	++taille;
	lesRegroup[ligne][0]=taille;
	for(j=1;j<=taille;++j){
	  lesRegroup[ligne][j]=j;
	}
      }
    }
  }
  //Il faut voir si on garde la derniere entree
  actuel = 0;
  for(j=0;j<lesRegroup[ligne][0];++j){
    actuel += p->demande[ lesRegroup[ligne][j+1]];
  }
  if(actuel<=p->capacite){
    ++ligne;
  }
  lesRegroup[ligne][0]=0;
  return lesRegroup;
}
void lectureReg(int** reg){
  int i,ligne;
  ligne = 0;
  while(reg[ligne][0]!=0){
    for(i=1;i<=reg[ligne][0];++i){
      printf("%d, ",reg[ligne][i]); 
    }
    ++ligne;
    puts("");
  }
}
/* int** allPermut(int* chem){ */
/*   int** permut; */
/*   int taille = chem[0]; */
/*   permut=(int **) malloc (taille-1 * sizeof(int *)); */
/*   for(i=0;i < taille;++i) lesRegroup[i] = (int *) malloc ( (nblieux+1) * sizeof(int)); */
/* } */

/* lecture des donnees */ 
void lecture_data(char *file, donnees *p)
{
	int i,j;
	FILE *fin;
	
	int val;
	fin = fopen(file,"rt");
	
	/* Lecture du nombre de villes */
	
	fscanf(fin,"%d",&val);
	p->nblieux = val;

	/* Allocation mémoire pour la demande de chaque ville, et le distancier */
	
	p->demande = (int *) malloc (val * sizeof(int));
	p->C = (int **) malloc (val * sizeof(int *));
	for(i = 0;i < val;i++) p->C[i] = (int *) malloc (val * sizeof(int));
	
	/* Lecture de la capacité */
	
	fscanf(fin,"%d",&val);
	p->capacite = val;
	
	/* Lecture des demandes des clients */
	
	for(i = 1;i < p->nblieux;i++)
	{
		fscanf(fin,"%d",&val);
		p->demande[i] = val;
	}
	
	/* Lecture du distancier */

	for(i = 0; i < p->nblieux; i++)
		for(j = 0; j < p->nblieux; j++)
		{
			fscanf(fin,"%d",&val);
			p->C[i][j] = val;
		}
		
	fclose(fin);
}

/* Fonction de libération mémoire des données */

void free_data(donnees *p)
{
	int i;
	for(i = 0;i < p->nblieux;i++) free(p->C[i]);
	free(p->C);
	free(p->demande);	
}


int main(int argc, char *argv[])
{	
	/* Déclarations des variables (à compléter) */

	donnees p; 
	double temps;
		
	/* Chargement des données à partir d'un fichier */
	
	lecture_data(argv[1],&p);
	
	/* Lancement de la résolution... */

	crono_start(); // .. et donc du chronomètre

	/* .... */




	lectureReg(enumererRegroupe(&p));







	/* ... */



	/* Problème résolu, arrêt du chrono */
	
	crono_stop();
	temps = crono_ms()/1000,0;
	
	/* Affichage des résultats (à compléter) */
	
	printf("Temps : %f\n",temps);	
	
	/* libération mémoire (à compléter en fonction des allocations) */

	free_data(&p);
	
	/* J'adore qu'un plan se déroule sans accroc! */
	return 0;
}
