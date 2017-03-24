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
//Le maillon
typedef struct maillon{
  trajet* traj;//Oblige? pour l insertion des nouveau
  struct maillon* suiv;
} maillTrajet;
typedef struct {
  maillTrajet* prem;
} teteTrajet;
//-------------Nb trajet totale possible Somme de i=1 a nblieux ( n!/(i!(n-i)!))-----------

void freeTrajet(trajet* t){
  int i;
  free(t->chemin);
}
void freeMaill(maillTrajet* m){
  maillTrajet* cour;
  maillTrajet* prec;
  cour = m->suiv;
  while(cour!=NULL){
    freeTrajet(cour->traj);
    cour->traj = NULL;
    //Faut il free le suiv, si oui comment
    prec = cour;
    cour = cour->suiv;
    free(prec);
  }
}
//Calcul d un factoriel, necessaire au nombre de regroupement max
int facto(int n){
  if (n==0)
    return 1;
  else
    return n*facto(n-1);
}

/* enumeration des regroupements */
int enumererRegroupe(donnees *p, maillTrajet* debut){
  int i,j,cmp;
  //On compte le nombre de trajet max
  //int totT=0;
  //p->nblieux semble compter le camp de base, or il n est pas dans les regroupement, a voir si ne pas le -1
  int nblieux = p->nblieux -1;
  /* 
         Ce total est bien joli, mais 15! fait 1.3*10^12 donc l entier est rapidement plein
	 Donc le malloc ne veut pas et ca fait de la merde
	 Donc essayons avec une liste chaine, surtout que sur les X millions de regroupements totaux existant seulement quelques uns etaient utilises
  */
  /* int factoNblieux= facto(nblieux);//Pour reduire le nombre de calcul */
  /* for(i=1;i<nblieux;++i) */
  /*   totT+= factoNblieux/( facto(i) * facto(nblieux-i)); */
  
  /* ++ totT;// Le regroupement de nblieux */
  //++totT;//Une case supplementaire au cas où on a tous les regroupement pour le 0
  //Il semblerait que le tableau aille jusque [totT] en partant de 0 donc totT+1 case
  /* int ** lesRegroup; */
  /* lesRegroup=(int **) malloc (totT * sizeof(int *)); */
  /* for(i=0;i < totT;++i) lesRegroup[i] = (int *) malloc ( (nblieux+1) * sizeof(int)); */
  //On a le tableau de dimension 2 pouvant tout contenir
  int taille, actuel, depart, ligne;
  //int bLigne, bDepart;//On les considere comme des booleens
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
  /* taille = 1; */
  /* ligne = 0; */
  /* lesRegroup[ligne][0]=taille; */
  /* lesRegroup[ligne][1]=1; */
  /*On va travailler en correction
    Donc on verifie la ligne du dessus pour savoir si on se met apres ou si on l ecrase(ligne++ ou non)
    On utilise la ligne du dessus pour savoir ce que contient la ligne actuel:
                              -Si ne finit pas par nblieux alors
			                   - on rajoute 1 a la derniere case
			                   -sinon la prochaine ligne augmente le nombre le plus loin a droite sans que ca depasse avec la taille (pos+(nblieux-nb))<taille+1 (avec nb le nombre a mettre dans la case pos
				       -Si le sinon arrive a 0 (case reserve a la taille) on augmente la taille et repart de 1 (simple boucle for pour remplir la ligne a 1 2 3 ..)
    Normalement la taille ne depassera pas la taille car le compte semble être bon, sinon sauter le for principal en while taille<nblieux
    
   */
  /* int lignePrec = 0; */
  /* int breakPoint,valBreak;//regarde a quel endroit on casse la chaine prec pour faire l actuel */
  /* while(taille<nblieux){ */
  /*   //On verifie que la ligne precedement ecrite ne depasse pas */
  /*   actuel = 0; */
  /*   for(j=0;j<lesRegroup[ligne][0];++j){ */
  /*     actuel += p->demande[ lesRegroup[ligne][j+1]]; */
  /*   } */
  /*   lignePrec=ligne; */
  /*   if(actuel<=p->capacite){ */
  /*     ++ligne; */
  /*   } */
  /*   if(lesRegroup[ lignePrec ][taille] != nblieux){//Si le dernier n'est pas nblieux */
  /*     //On suppose que l'on ne sera pas au dessus car ca ne doit pas arriver */
  /*     //((ligne==0) ? 0 : (ligne-1)): Cette merde sert a utiliser ligne-1 sans tomber dans le negatif */
  /*     //Utilise un prec IDIOT */
  /*     lesRegroup[ ligne ][0]=taille; */
  /*     for(j=1;j<lesRegroup[lignePrec][0];++j){ */
  /* 	lesRegroup[ligne][j]=lesRegroup[lignePrec][j]; */
  /*     } */
  /*     lesRegroup[ligne][taille]=lesRegroup[lignePrec][taille]+1; */
      
  /*   }else{ */
  /*     //On sait que la derniere case est le nombre max */
  /*     breakPoint=taille-1; */
  /*     while(breakPoint>0 && lesRegroup[lignePrec][breakPoint]+1+taille-breakPoint>nblieux){ */
  /* 	--breakPoint; */
  /*     } */
  /*     if(breakPoint>0){ */
  /* 	lesRegroup[ligne][0]=taille; */
  /* 	for(j=1;j<breakPoint;++j){ */
  /* 	  lesRegroup[ligne][j]=lesRegroup[lignePrec][j]; */
  /* 	} */
  /* 	valBreak = lesRegroup[lignePrec][breakPoint]+1; */
  /* 	lesRegroup[ligne][breakPoint]= valBreak; */
  /* 	for(j=1;j<=taille-breakPoint;++j){ */
  /* 	  lesRegroup[ligne][breakPoint+j]=valBreak+j; */
  /* 	} */
  /*     }else{ */
  /* 	++taille; */
  /* 	lesRegroup[ligne][0]=taille; */
  /* 	for(j=1;j<=taille;++j){ */
  /* 	  lesRegroup[ligne][j]=j; */
  /* 	} */
  /*     } */
  /*   } */
  /* } */
  /* //Il faut voir si on garde la derniere entree */
  /* actuel = 0; */
  /* for(j=0;j<lesRegroup[ligne][0];++j){ */
  /*   actuel += p->demande[ lesRegroup[ligne][j+1]]; */
  /* } */
  /* if(actuel<=p->capacite){ */
  /*   ++ligne; */
  /* } */
  /* lesRegroup[ligne][0]=0; */
  /* return lesRegroup; */
  int remplissage;//Combien le drones aurait il d eau apres le passage sur le point courant
  trajet* prec;
  trajet* cour;
  maillTrajet* res;//Suit l endroit d insertion du trajet
  int acc;
  int breakPt;
  maillTrajet* Mprec= debut;//On place le pointeur au debut, donc si il y avait qqch cela est perdu
  //on introduit pas la premiere ligne d abord
  //Quoi que.. on pourrait essayer d inserer le premier point qui rentre (rapport reservoir), si on en pas au moins c'est regle
  //peut prmettre aussi d eliminer des points mais bonne chance
  i=1;
  while(p->demande[i]>p->capacite && i<=nblieux)
    ++i;
  if(i>nblieux){
    printf("Le reservoir est trop petit pour prendre un trou");
    return;
  }
  cmp = 1;
  cour = (trajet*) malloc (sizeof (trajet));
  cour->nbplace = 1;
  cour->chemin = (int *) malloc (1 * sizeof (int));
  cour->chemin[0] = i;
  Mprec->traj = cour;
  prec = cour;
  taille = 1;//On commence avec les trajet de 1
  while(taille<=nblieux){//Normalement le dernier trajet que l on va trouver est celui passant par tout les points
    //A changer si on trouve comment optimiser avec des appartenance
    //ie: si un chemin avec les point xyz ne rentre pas, on ignore les chemins qui ont xyz
    //semi opti=> si le prefix depasse on saute a la suite <= plus simple (on augmente le dernier du prefixe ou la taille si dernier = nblieux)
    /* if(prec==NULL){ */
    /*   breakPt = -1; */
    /* }else  */if(prec->chemin[ taille-1 ] < nblieux ){//Si la derniere du regroupement precedent n est pas la valeur max
      breakPt = taille-1;//A voir si le bP est devant ou dessus le changement     
    }else{
      //Recherche du bP
      breakPt=taille-1; 
      while(breakPt>=0 && prec->chemin[breakPt]+1+taille-breakPt-1>nblieux){
	//Je vote dessus
  	--breakPt;
      }
      //Besoin de ça ici pour faire passer la premiere ligne sans tout faire sauter
      if(breakPt<0)
	++taille;
    }
    //Donc on a le bP
    //On cree le nouveau trajet
    cour = (trajet*) malloc (sizeof (trajet));
    cour->chemin = (int *) malloc (taille * sizeof (int));
    //on copie la partie d avant le bP
    remplissage = 0;
    i = 0;
    //Le calcul de remplissage est chiant ici car on sait deja que ca rentre a voir
    while(i<breakPt && remplissage<= p->capacite){
      //Si prec est NULL alors bP=0, on ne rentre pas dans le for
      cour->chemin[i] = prec->chemin[i];
      remplissage += p->demande[ prec->chemin[i] ];
      ++i;
    }
    //On met la fin
    //Pour le premier tour, prec est nul, il faut remplir avec 1
    //Ou pas avec le changement plus haut pour plus tard, terminons deja cela
    //Le || prec==NULL est la que pour le premier tour, faire changement au dessus
    acc = (breakPt<0 /* && taille>1 */ /* || prec==NULL */) ?  1 : prec->chemin[breakPt]+1;
    i=(remplissage <= p->capacite)? breakPt : i;
    if(i<0)
      i=0;
    while(i<taille && remplissage<= p->capacite){
      cour->chemin[i] = acc;
      remplissage += p->demande[ acc ];
      ++i;
      ++acc;
    }
    //Est ce que mettre remplissage> p->capacite dans un "booleen" aiderait?
    if(remplissage > p->capacite){
      //Cela veut dire que tout est bien rempli jusqu a i-1
      //ie: le prefixe chemin[0..i-1] depasse le reservoir mais pas chemin[0..i-2]
      //On veut donc que lors du prochain tour le bP tombe sur i-1 ou plus bas (suivant la config)
      //On rempli donc toutes les cases suivantes avec nblieux pour l obliger a traverser jusque i-1
      //BTW=> i-1 car il y a un ++i apres le remplissage, sinon a mettre dans le if, a voir
      for(acc = i;acc<taille;++acc){
	cour->chemin[acc] = nblieux;
      }
    }else{
      //Le chemin est acceptable, on le rajoute au resultat
      ++cmp;
      res = (maillTrajet*) malloc (sizeof (maillTrajet));
      res->traj = cour;
      cour->nbplace = taille;
      cour->longueur = 0;
      //res->suiv = (maillTrajet*) malloc (sizeof (maillTrajet));
      Mprec->suiv = res;
      Mprec = res;
      //Le premier trajet est peut etre vide
    }
    prec = cour;
  }
  return cmp;
}
//Besoin d un completement nouveau vu que l on ne range plus de la meme maniere
/* void lectureReg(int** reg){ */
/*   int i,ligne; */
/*   ligne = 0; */
/*   while(reg[ligne][0]!=0){ */
/*     for(i=1;i<=reg[ligne][0];++i){ */
/*       printf("%d, ",reg[ligne][i]);  */
/*     } */
/*     ++ligne; */
/*     puts(""); */
/*   } */
/* } */
//On peut meme lire les permutation avec, cool
void lectureReg(maillTrajet* deb){
  //On ne remplit pas le premier maillon, pas super
  //Ne va pas afficher la premiere permut, il faut faire le truc pour le premier regroup
  maillTrajet*  cour = deb/* ->suiv */;
  int i;
  while(cour!=NULL){
    if(cour->traj != NULL){//Sortez protege
      for(i=0;i<cour->traj->nbplace;++i){
	printf("%d, ",cour->traj->chemin[i]);
      }
      puts("");
    }
    cour = cour->suiv;
  }
}
//On fait les modifs pour la chaine en direct, donc ca va planter
//int** allPermut(int* chem){
//On met dans les permut de t en chaine dans debut pour 
void allPermut(trajet *t, maillTrajet *debut){
  //Fait en suivant l algo de Even
  //Heap est peut etre mieux
  /* int** permut; */
  int taille = t->nbplace;
  /* int tot = facto(taille)+1; */
  //On met un stop a la fin
  /* permut=(int **) malloc (tot * sizeof(int *)); */
  int i;
  /* for(i=0;i < tot;++i) permut[i] = (int *) malloc ( taille * sizeof(int)); */
  int* sgn = (int *) malloc (taille * sizeof(int));
  //Le tableau contenant les signes de deplacement
  int* tmpChem = (int *) malloc (taille * sizeof(int));
  //Le tableau contenant la permutation en cours
  
  /* tmpChem[0]=t->chemin[0]; */
  //On fait le premier a cote pour utiliser le meme for que sgn
  int nbNZero = 0;
  /* int ligne = 0; */
  trajet * cour;
  maillTrajet* Mprec = debut;
  cour = (trajet *) malloc (sizeof (trajet));
  cour->chemin = (int *) malloc (taille * sizeof (int));
  /* permut[ligne][0]= tmpChem[0]; */
  //De meme, on mets aa premiere la ligne
  sgn[0]=0;
  tmpChem[0]=t->chemin[0];
  cour->chemin[0]= tmpChem[0];
  for(i=1;i<taille;i++){
    sgn[i]=-1;
    ++nbNZero;
    tmpChem[i]=t->chemin[i];
    cour->chemin[i]= tmpChem[i];
  }
  cour->nbplace = taille;
  Mprec->traj = cour;
  Mprec->suiv = NULL;
  maillTrajet* res;
  /* ++ligne; */
  int posMax = taille-1;//Notre tableau est dans l ordre croissant par construction
  //Sinon on peut ajouter la recherche du max dans le for du sgn
  int tmp;
  int newPMax=posMax;
  int valMax;
  while(nbNZero >0){
    if(sgn[posMax]<0){
      tmp = tmpChem[posMax];
      tmpChem[posMax]=tmpChem[posMax-1];
      tmpChem[posMax-1]=tmp;
      tmp = sgn[posMax];
      sgn[posMax]= sgn[posMax-1];
      sgn[posMax-1]=tmp;
      --posMax;
    }else if(sgn[posMax]>0){
      tmp = tmpChem[posMax];
      tmpChem[posMax]=tmpChem[posMax+1];
      tmpChem[posMax+1]=tmp;
      tmp = sgn[posMax];
      sgn[posMax]= sgn[posMax+1];
      sgn[posMax+1]=tmp;
      ++posMax;
    }else{
      //Normalement on ne verra jamais mais je souhaite tester
      printf("On a un sgn posmax nul, %d", posMax);
    }
    if(posMax == 0 || posMax == taille-1 || sgn[posMax]>0 && tmpChem[posMax]<tmpChem[posMax+1] || sgn[posMax]<0 && tmpChem[posMax]<tmpChem[posMax-1]){
      sgn[posMax]=0;
      --nbNZero;
    }
    //On change le signe de la premiere partie du tableau
    //ie: si plus grand et au debut alors +1
    cour = (trajet *) malloc (sizeof (trajet));
    cour->nbplace = taille;
    cour->chemin = (int *) malloc (taille * sizeof (int) );
    for(i=0;i<posMax;++i){
      tmp = tmpChem[i];
      if(tmp>tmpChem[posMax]){
	//si le signe etait zero alors on a un non zero de plus, sinon pas de changement
	//Mais je suis presque sur qu ils doivent tous etre a zero
	//sinon on les aurait pris en tant que max
	nbNZero= sgn[i]==0 ? ++nbNZero : nbNZero;
	sgn[i]= 1;
      }
      cour->chemin[i]=tmp;
    }
    cour->chemin[posMax]= tmpChem[posMax];
    //On change le signe de la seconde partie du tableau
    //ie: si plus grand et au debut alors -1

    for(i=posMax+1;i<taille;++i){
      tmp = tmpChem[i];
      if(tmp>tmpChem[posMax]){
	nbNZero= sgn[i]==0 ? ++nbNZero : nbNZero;
	sgn[i]= -1;
      }
      cour->chemin[i]=tmp;
    }
    valMax = 0;
    newPMax = -1;
    //On cherche le nouveau posMax
    for(i=0;i<taille;++i){
      if(tmpChem[i]>valMax && sgn[i]!=0){
	newPMax = i;
	valMax = tmpChem[i];
      }
    }
    if(newPMax==-1 && nbNZero>0){
      printf("MEEEEEEERRRRRRRRRRDDDDDDDDDEEEEEEEEEEEE");
    }else{
      posMax = newPMax;
    }
    //On affecte la nouvelle valeur
    /* ++ligne; */
    res = (maillTrajet*) malloc (sizeof (maillTrajet));
    res->traj = cour;
    //res->suiv = (maillTrajet*) malloc (sizeof (maillTrajet));
    Mprec->suiv = res;
    Mprec = res;
    Mprec->suiv = NULL;
    
  }
  /* permut[ligne][0]= -1; */
  //Le stop est pose
  /* return permut; */
}
void lecturePer(int** permut, int taille){
  int i;
  int ligne = 0;
  while(permut[ligne][0]!=-1){
    for(i=0;i<taille;++i){
      printf("%d, ",permut[ligne][i]);
    }
    puts("");
    ++ligne;
  }
}
int longueur(int *chemin, int taille, donnees *p){
  int res = 0;
  int dep = 0;
  int i;
  for(i=0;i<taille;++i){
    res+=p->C[dep][ chemin[i] ];
    dep = chemin[i];
  }
  //Retour a la base
  res+=p->C[dep][0];
  return res;
}
/* int bestLength(int ** permut, int taille, donnees *p){ */
/*   int ligne = 0; */
int bestLength(maillTrajet* t, donnees *p){
  /* Cool mais necessite une library supp
     int min = std::numeric_limits<int>::max();*/
  maillTrajet* cour = t;
  //Toute les permutations ont la même taille
  int taille = cour->traj->nbplace;
  int min = longueur(cour->traj->chemin,taille,p);
  int tmp;
  cour = cour->suiv;
  while(cour != NULL){
    tmp = longueur(cour->traj->chemin,taille,p);
    if(tmp<min){
      min = tmp;
    }
    cour = cour->suiv;
    //++ligne;
  }
  return min;
}
//lecture des donnees 
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
	//Les test pour avec les int **
	/* int** regroup; */
	/* int** permut; */
	/* regroup = enumererRegroupe(&p); */

	/* lectureReg(regroup); */
	/* printf("-----------------------------"); */
	/* puts(""); */
	/* int ligne = 0; */
	/* while(regroup[ligne][0]>0){ */
	/*   permut=allPermut(regroup[ligne]); */
	/*   lecturePer(permut,regroup[ligne][0]); */
	/*   printf("Chemin le plus court: %d",bestLength(permut,regroup[ligne][0],&p)); */
	/*   puts(""); */
	/*   ++ligne; */
	/* } */
	/* //lecturePer(allPermut(regroup[15]),3); */

	//Avec les chaines
	maillTrajet deb;
	int i;
	int tmp;
	//printf("%d",enumererRegroupe(&p,&deb));
	int nbReg = enumererRegroupe(&p,&deb);
	int nbCreux = 0;
	/* puts(""); */
	//lectureReg(&deb);
	maillTrajet permut;
	maillTrajet* cour;
	cour = &deb;
	/* printf("------------------------------------"); */
	/* puts(""); */
	while(cour != NULL){
	  if(cour->traj != NULL){
	    allPermut(cour->traj,&permut);
	    /* //lecture du chemin */
	    /* for(i=0;i<cour->traj->nbplace;++i){ */
	    /*   printf("%d, ",cour->traj->chemin[i]); */
	    /* } */
	    /* puts(""); */
	    tmp =  bestLength(&permut,&p);
	    /* printf("Chemin le plus court: %d",tmp); */
	    cour->traj->longueur = tmp;;
	    nbCreux+= cour->traj->nbplace;
	    /* puts(""); */
	    //freeMaill(&permut);
	  }else{
	    printf("????");
	  }
	  cour = cour->suiv;
	}
	/* trajet t; */
	/* maillTrajet permut9; */
	/* int taille9 = 5; */
	/* t.chemin = (int *) malloc (taille9 * sizeof (int)); */
	/* //int i; */
	/* for(i=0;i<taille9;++i) */
	/*   t.chemin[i]=i+1; */
	/* t.nbplace = taille9; */
	/* //CeBo */
	/* allPermut(&t,&permut9); */
	/* lectureReg(&permut9); */
	//On a dans deb tous les regroupements et leurs longueur min
	/* printf("------------------G------L------P------K------------------"); */
	glp_prob *prob;
	
	int *ja;
	int *ia;
	double *ar;

	char **nomContr;
	char **numero;
	char **nomvar;

	double z;
	double *x;

	prob = glp_create_prob();

	glp_set_prob_name(prob,"En Drone pour tous, tous pour l eau");
	glp_set_obj_dir(prob, GLP_MIN);
	
	//-1 car le 0 n a pas de contrainte
	//Le -1 a se promener me fait 
	int nblieux = p.nblieux -1;
	glp_add_rows(prob,nblieux);
	nomContr = (char **) malloc (nblieux * sizeof(char *));
	//Numùero utile car strcat a besoin de deux char*
	numero = (char **) malloc (nblieux * sizeof(char *));
	for(i=1;i<=nblieux;i++){
	  //On ne peut pas numeroter plus loin que 99 ='( (faudrait des if+ math)
	  nomContr[i-1] = (char *) malloc (8 * sizeof(char));
	  numero[i-1] = (char *) malloc (3 * sizeof(char));
	  strcpy(nomContr[i-1],"point");
	  sprintf(numero[i-1], "%d", i);
	  strcat(nomContr[i-1], numero[i-1]);
	  //Les noms seront "point1", "point2",...
	  glp_set_row_name(prob,i,nomContr[i-1]);
	  //A la vue du nombre de i-1, autant faire i de 0 a n-1 avec i+1 dans ro_name
	  glp_set_row_bnds(prob,i, GLP_FX, 1.0, 0.0);
	}
	//Autant de colonne que de var de decision que de regroupements
	glp_add_cols(prob,nbReg);
	nomvar = (char **) malloc (nbReg * sizeof(char *));
	for(i = 1; i<=nbReg;++i){
	  nomvar[i-1] = (char*) malloc (4 * sizeof(char));
	  sprintf(nomvar[i-1],"x%d",i);
	  glp_set_col_name(prob,i,nomvar[i-1]);
	  glp_set_col_bnds(prob,i,GLP_DB,0.0,1.0);
	  glp_set_col_kind(prob,i,GLP_BV);
	}
	  

	//La matrice creuse
	//On aura autant de creux que la somme des nbplace
	//Le tableau aurait simplifier le truc ou pas
	//En fait on fait le calcul en meme temps que la recherche de longueur
	ia = (int *) malloc (nbCreux * sizeof(int));
	ja = (int *) malloc (nbCreux * sizeof(int));
	ar = (double *) malloc (nbCreux * sizeof(double));
	int colonne = 1;
	int pos = 1;
	cour = &deb;
	while(cour != NULL){
	  for(i=0;i<cour->traj->nbplace;++i){
	    ia[pos] = cour->traj->chemin[i];
	    ja[pos] = colonne;
	    ar[pos] = 1.0;
	    ++pos;
	  }
	  glp_set_obj_coef(prob,colonne,cour->traj->longueur);
	  cour = cour->suiv;
	  ++colonne;
	}
	glp_load_matrix(prob,nbCreux,ia,ja,ar);
	glp_write_lp(prob,NULL,"drone.lp");

	glp_simplex(prob,NULL);
	glp_intopt(prob,NULL);

	/* Problème résolu, arrêt du chrono */
	
	crono_stop();
	temps = crono_ms()/1000,0;
	
	/* Affichage des résultats (à compléter) */
	z = glp_mip_obj_val(prob);
	x = (double *) malloc (nbReg * sizeof(double));
	for(i=0;i<nbReg;++i) x[i] = glp_mip_col_val(prob,i+1);

	printf("z = %lf\n",z);
	for(i=0;i<nbReg;++i) printf("x%d = %d, ", i, (int)(x[i]+0.5));
	puts("");

	printf("Temps : %f\n",temps);	
	
	/* libération mémoire (à compléter en fonction des allocations) */
	glp_delete_prob(prob);
	freeMaill(&deb);
	free_data(&p);
	
	/* J'adore qu'un plan se déroule sans accroc! */
	return 0;
}
