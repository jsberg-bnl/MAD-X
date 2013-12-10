#include "madx.h"

static int
find_index_in_table(char *cols[], const char *name )
{
  for (int i = 0; strcmp(cols[i], " "); i++)
    if (string_icmp(cols[i], name) == 0)
      return i;

  return -1; // not found
}

static void
pro_error_make_efield_table(void)
{
  struct table *ttb = efield_table;
  struct node *nanf;
  struct node *nend;
  int    j;
  struct sequence* mysequ = current_sequ;

  nanf = mysequ->ex_start;
  nend = mysequ->ex_end;

      while (nanf != nend) {
        if(nanf->sel_err == 1) {
           string_to_table_curr("efield","name",nanf->name);
/* */
                  /*
           printf("=> %s %e %e %e\n",nanf->name,nanf->p_fd_err,nanf->p_al_err);
                  */
           if(nanf->p_fd_err != NULL) {
              int from_col = find_index_in_table(efield_table_cols, "k0l");
              int to_col = find_index_in_table(efield_table_cols, "k20sl");
              int ncols = to_col - from_col + 1;
              for (j=0; j < ncols; j++) {
                 ttb->d_cols[from_col+j][ttb->curr] = nanf->p_fd_err->a[j];
                  /*
                 printf("Field: %d %e\n",j,ttb->d_cols[j][ttb->curr]);
                  */
              }
           }
           if(nanf->p_al_err != NULL) {
              int from_col = find_index_in_table(efield_table_cols, "dx");
              int to_col = find_index_in_table(efield_table_cols, "mscaly");
              int ncols = to_col - from_col + 1;
              for (j=0; j < ncols; j++) {
                 ttb->d_cols[from_col+j][ttb->curr] = nanf->p_al_err->a[j];
                  /*
                 printf("Align: %d %e\n",j,ttb->d_cols[j][ttb->curr]);
                  */
              }
           }
           /* AL: RF-errors */
           if(nanf->p_ph_err != NULL) {
              ttb->d_cols[find_index_in_table(efield_table_cols, "rfm_freq")][ttb->curr] = nanf->rfm_freq;
              ttb->d_cols[find_index_in_table(efield_table_cols, "rfm_harmon")][ttb->curr] = nanf->rfm_harmon;
              ttb->d_cols[find_index_in_table(efield_table_cols, "rfm_lag")][ttb->curr] = nanf->rfm_lag;
              int from_col = find_index_in_table(efield_table_cols, "p0l");
              int to_col = find_index_in_table(efield_table_cols, "p20sl");
              int ncols = to_col - from_col + 1;
              for (j=0; j < ncols; j++) {
                 ttb->d_cols[from_col+j][ttb->curr] = nanf->p_ph_err->a[j];
                  /*
                 printf("Align: %d %e\n",j,ttb->d_cols[j][ttb->curr]);
                  */
              }
           }
/* */
           augment_count("efield");
        }
        nanf = nanf->next;
      }
}

static void
error_seterr(struct in_cmd* cmd)
{

/* read the errors from a named table  and stores
   them in the nodes of the sequence.       
   Subsequent Twiss will use them correctly.
   ===> Must be preceded by a call to "read_table"
   ===> (unless table exists in memory !)
*/

  int from_col, to_col, row, col, i;

  struct node *node, *node_end;

  char     name[NAME_L];
  char   slname[NAME_L];

  char     nname[NAME_L];
  char   slnname[NAME_L];

  char    *namtab, namtab_buf[NAME_L];
  int      t1;

  struct   table *err;

/* set up pointers to current sequence for later use */
  struct sequence* mysequ = current_sequ;
  node     = mysequ->ex_start;
  node_end = mysequ->ex_end;

  if ((namtab = command_par_string("table",cmd->clone)) != NULL) {
    printf("Want to use named table: %s\n",namtab);
    if ((t1 = name_list_pos(namtab, table_register->names)) > -1)
      printf("The table ==> %s <=== was found \n",namtab);
    else {
      warning("No such error table in memory:", namtab);
      exit(-77);
    }
  }
  else {
    if (get_option("debug")) {
      printf("No table name requested\n");
      printf("Use default name\n");
    }

    strcpy(namtab=namtab_buf,"error");
    if ((t1 = name_list_pos(namtab, table_register->names)) > -1)
      printf("The default table ==> %s <=== was found \n",namtab);
    else {
      warning("No default error table in memory:", namtab);
      exit(-77);
    }
  }
 
  err = table_register->tables[t1];

  for (row = 1; row <= err->curr; row++) {
    if (string_from_table_row(namtab, "name", &row, name)) break;

    // probably useless...
    stolower(name);
    strcpy(slname,strip(name));
    supp_tb(slname);

    for (node = mysequ->ex_start; node != node_end; node = node->next) {
    // probably useless...
      strcpy(nname,node->name);
      stolower(nname);
      strcpy(slnname,strip(nname));
      supp_tb(slnname);

      if(strcmp(slname, slnname) == 0) break;
    }

    /* We have now the input and the node, generate array and selection flag */
    if (!strcmp(slname, slnname)) {
      node->sel_err = 1;
      node->p_fd_err = new_double_array(FIELD_MAX); // zero initialized
      node->p_fd_err->curr = FIELD_MAX;
      node->p_al_err = new_double_array(ALIGN_MAX); // zero initialized
      node->p_al_err->curr = ALIGN_MAX;
      node->p_ph_err = new_double_array(RFPHASE_MAX); // zero initialized
      node->p_ph_err->curr = RFPHASE_MAX;

      from_col = find_index_in_table(efield_table_cols, "k0l");
      to_col   = find_index_in_table(efield_table_cols, "k20sl");
      if (from_col > 0 && to_col > 0)
        for (i=0, col=from_col; col <= to_col && col < err->num_cols; col++, i++)
          node->p_fd_err->a[i] = err->d_cols[col][row-1];

      from_col = find_index_in_table(efield_table_cols, "dx");
      to_col   = find_index_in_table(efield_table_cols, "mscaly");
      if (from_col > 0 && to_col > 0)
        for (i=0, col=from_col; col <= to_col && col < err->num_cols; col++, i++)
          node->p_al_err->a[i] = err->d_cols[col][row-1];

      col = find_index_in_table(efield_table_cols, "rfm_freq");
      node->rfm_freq   = col < 0 || col >= err->num_cols ? 0 : err->d_cols[col][row-1];

      col = find_index_in_table(efield_table_cols, "rfm_harmon");
      node->rfm_harmon = col < 0 || col >= err->num_cols ? 0 : err->d_cols[col][row-1];

      col = find_index_in_table(efield_table_cols, "rfm_lag");
      node->rfm_lag    = col < 0 || col >= err->num_cols ? 0 : err->d_cols[col][row-1];

      from_col = find_index_in_table(efield_table_cols, "p0l");
      to_col   = find_index_in_table(efield_table_cols, "p20sl");
      if (from_col > 0 && to_col > 0)
        for (i=0, col=from_col; col <= to_col && col < err->num_cols; col++, i++)
          node->p_ph_err->a[i] = err->d_cols[col][row-1];
    }
  }
}

static void
error_esave(struct in_cmd* cmd)
{
    char *ef_table_file;
/*  if(efield_table == NULL) { */
       efield_table = make_table("efield", "efield", efield_table_cols,
                               efield_table_types, 10000);
       add_to_table_list(efield_table, table_register);
       pro_error_make_efield_table();
/*  }                          */
    ef_table_file = command_par_string("file",cmd->clone);
    out_table("efield",efield_table,ef_table_file);
}

static void
error_ealign(struct in_cmd* cmd)
{
  struct node *ndexe;
  struct node *nextnode;
  int i;
  int chcount[3] = {0,0,0};
  double val[ALIGN_MAX] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  static char att[ALIGN_MAX][7] = {"dx","dy","ds","dphi","dtheta","dpsi","mrex","mrey","mredx","mredy","arex","arey","mscalx","mscaly"};

  struct command_parameter_list* pl = current_error->par;
  struct sequence* mysequ = current_sequ;

  nextnode = mysequ->ex_start;
  ndexe = mysequ->ex_end;

  while (nextnode != ndexe) {

      if(nextnode->sel_err == 1) {
        if(nextnode->p_al_err == NULL) {
          chcount[0]++;
          nextnode->p_al_err = new_double_array(ALIGN_MAX);
          nextnode->p_al_err->curr = ALIGN_MAX;
        } else {
          if(add_error_opt == 1) {
              chcount[2]++;
          } else {
              chcount[1]++;
          }
        }
          for(i=0;i<ALIGN_MAX;i++){
               val[i] = pl->parameters[i]->double_value;
               if(pl->parameters[i]->expr != NULL) {
                  if(pl->parameters[i]->expr->status != 1) {
                      val[i] = command_par_value(att[i], cmd->clone);
                      pl->parameters[i]->expr->status = 0;
                  }
               }
               if(add_error_opt == 1) {
                  nextnode->p_al_err->a[i] += val[i];
               } else {
                  nextnode->p_al_err->a[i] = val[i];
               }
          }
      }  /* end of treatment of selected node */
      nextnode = nextnode->next;
  }  /* end of loop over all nodes */
  if(chcount[0] != 0)
  fprintf(prt_file, "Assigned alignment errors to %d elements\n",chcount[0]);
  if(chcount[1] != 0)
  fprintf(prt_file, "Replaced alignment errors for %d elements\n",chcount[1]);
  if(chcount[2] != 0)
  fprintf(prt_file, "Added alignment errors to %d elements\n",chcount[2]);

}

static void
error_eprint(struct in_cmd* cmd)
{
  struct node *ndexe;
  struct node *nextnode;
  static char pln_alig[ALIGN_MAX][7] = {"dx","dy","ds","dphi","dtheta","dpsi","mrex","mrey","mredx","mredy","arex","arey","mscalx","mscaly"};
  static float alig_fact[ALIGN_MAX] = {1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1.0,1.0};

  int i;
  struct sequence* mysequ = current_sequ;
  int mycount;
  int fll;

  ndexe = mysequ->ex_end;
  nextnode = mysequ->ex_start;

  mycount = 0;
 
  fll = command_par_value("full", cmd->clone);

  while (nextnode != ndexe) {

    if((nextnode->sel_err == 1) || (fll > 0) ){
       if(nextnode->p_al_err != NULL) {
         fprintf(prt_file, "\n\nAlignment errors for element %s \n",nextnode->name);
         fprintf(prt_file,"\nDisplacements in [mm], rotations in [mrad] \n");
         fprintf(prt_file," %6s %10s %12s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s\n",
                  pln_alig[0], pln_alig[1],pln_alig[2],pln_alig[3],
                  pln_alig[4], pln_alig[5],pln_alig[6],pln_alig[7],
                  pln_alig[8], pln_alig[9],pln_alig[10],pln_alig[11],
                  pln_alig[12],pln_alig[13]);
         for(i=0;i<nextnode->p_al_err->curr;i++) { 
            fprintf(prt_file, "%10.6f ",alig_fact[i]*nextnode->p_al_err->a[i]);
         }
         fprintf(prt_file, "\n");
       } else {
         fprintf(prt_file, "\n\nAlignment errors for element %s \n",nextnode->name);
         fprintf(prt_file,"\nDisplacements in [mm], rotations in [mrad] \n");
         fprintf(prt_file," %6s %10s %12s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s %10s\n",
                  pln_alig[0], pln_alig[1],pln_alig[2],pln_alig[3],
                  pln_alig[4], pln_alig[5],pln_alig[6],pln_alig[7],
                  pln_alig[8], pln_alig[9],pln_alig[10],pln_alig[11],
                  pln_alig[12],pln_alig[13]);
         fprintf(prt_file, "\n");
       }

       if(nextnode->p_fd_err != NULL) {
         mycount++;
         if(mycount <= 50000) {
           /* fprintf(prt_file,"%s %d\n",nextnode->name,(int)nextnode->p_fd_err); */
           fprintf(prt_file, "\n\nField errors for element %s \n",nextnode->name);
           fprintf(prt_file, "Multipole order:     Normal:           Skew: \n");
           for(i=0;i<EFIELD_TAB;i++) {
              fprintf(prt_file, "%8d          %8e      %8e\n",i/2,
                     nextnode->p_fd_err->a[i],
                     nextnode->p_fd_err->a[i+1]);
              i++;
           }
           fprintf(prt_file, "\n");
         }
       } else {
         mycount++;
         if(mycount <= 50000) {
           fprintf(prt_file, "\n\nField errors for element %s \n",nextnode->name);
           fprintf(prt_file, "Multipole order:     Normal:           Skew: \n");
         }
       }
    }

      nextnode = nextnode->next;
  }

}

static void
error_efcomp(struct in_cmd* cmd)
{
  //  struct name_list* nl;
  //  int pos;
  
  //  struct node *ndexe;
  //  struct node *nextnode;
  //  int    lvec;
  int hyst = 0;
  //  int    flgmgt = 0;
  int chcount[3] = {0,0,0};
  const char *rout_name = "error_efcomp";
  //  double norfac; /* factor for normalization at reference radius */
  int    n = -1;     /* order of reference multipole */
  double rr = 0.0;   /* reference radius for multipole error */
  double rrr = 0.0;  /* reference radius for multipole error */
  double freq = 0.0; /* frequency for RF-Multiples */
  int harmon = 0;    /* harmonic number for RF-Multipoles */
  double lag = 0.0;  /* lag for RF-Multipoles */
  //  struct double_array *ptr;
  //  struct double_array *pcoef;
  double h_co_n[FIELD_MAX/2][4];
  double h_co_s[FIELD_MAX/2][4];
  //  double *hco_n;
  //  double *hco_s;
  //  double *nvec;
  //  double deer;  
  //  double ref_str;
  //  double ref_strn;
  //  double ref_len;
  //  double nlength;
  //  double nvec0, nvec1, nvec2, nvec3;
  //  double val[4] = {0, 0, 0, 0};
  static const char *atts[6] = {"order","radius","hyster","rfm_freq", "rfm_harmon", "rfm_lag"};
  static const char *attv[6] = {"dkn","dks","dknr","dksr","dpn","dps"};
  const size_t attv_len = sizeof attv/sizeof *attv;
  const size_t atts_len = sizeof atts/sizeof *atts;
  int iattv[attv_len];
  struct sequence* mysequ = current_sequ;
  
  // double *nvec = mycalloc_atomic("error_efcomp", 1000, sizeof *nvec);

  struct node *ndexe = mysequ->ex_end;
  struct node *nextnode = mysequ->ex_start;
  
  struct name_list* nl = cmd->clone->par_names;

  const int opt_debug = get_option("debug");
  
  /* here comes a kludge, check which of the assignment vectors is there */
  /*
    i = 0;
    while((cmd->tok_list->p[i]) != NULL) {
      for(k=0;k<4;k++)
        if(strcmp(cmd->tok_list->p[i],attv[k]) == 0)
          iattv[k] = 1;
      ++i;
    }
  */
  for(unsigned int k=0; k<attv_len; k++) {
    iattv[k] = 0;
    int pos = name_list_pos(attv[k],nl);
    if(nl->inform[pos] > 0) {
      if (opt_debug)
        	fprintf(prt_file, "set iattv %d for %s to 1\n",iattv[k],attv[k]);
      iattv[k] = 1;
    }
  }
  
  for(int i=0;i<FIELD_MAX/2;i++) {
    for(int j=0;j<4;j++) {
      h_co_n[i][j] = 0.0;
      h_co_s[i][j] = 0.0;
    }
  }
  
  while (nextnode != ndexe) { /*loop over elements and get strengths in vector*/
    current_node = nextnode;
    int flgmgt = node_value("magnet");
    if((nextnode->sel_err == 1) && (flgmgt == 1))  {
      if(nextnode->p_fd_err == NULL) {
	      chcount[0]++;
	      nextnode->p_fd_err = new_double_array(FIELD_MAX);
	      nextnode->p_fd_err->curr = FIELD_MAX;
      } else {
	      if(add_error_opt == 1) chcount[2]++; else chcount[1]++;
      }
      
      if (opt_debug)
        fprintf(prt_file, "field for %s %s %d\n", nextnode->name, nextnode->base_name, nextnode->sel_err);
      
      /* now get order (n), radius (rr) and hyster flag (hyst) from command, if any */
      /* AL: added 'freq' option for RF-Multipoles */
      for(unsigned int i=0; i<atts_len; i++) {
        double val = command_par_value(atts[i],cmd->clone);

        switch (i) {
          case 0:
            n = val;
            if (opt_debug) fprintf(prt_file, "order  is %d\n",n);
            break;

          case 1:
            rrr = val;
            rr  = fabs(rrr);
            if (opt_debug) fprintf(prt_file, "radius is %f\n",val);
            break;

          case 2:
            hyst = val;
            if (opt_debug) fprintf(prt_file, "hyster flag is %d\n",(int)val);
            break;

          case 3:
            freq = val;
            nextnode->rfm_freq = freq;
            if (opt_debug) fprintf(prt_file, "freq flag is %d\n",(int)val);
            break;

          case 4:
            harmon = (int)val;
            nextnode->rfm_harmon = harmon;
            if (opt_debug) fprintf(prt_file, "harmon flag is %d\n",(int)val);
            break;

          case 5:
            lag = lag;
            nextnode->rfm_lag = lag;
            if (opt_debug) fprintf(prt_file, "lag flag is %d\n",(int)val);
            break;
        }
      }
      
      /* now get coefficients for time memory effects in magnets */
      {
	      struct double_array *pcoef = command_par_array("hcoeffn",cmd->clone);
	      if(pcoef != NULL) {
	        double *hco_n = &h_co_n[0][0];
	        for(int j=0;j<pcoef->curr;j++) {
	          *hco_n = pcoef->a[j];
	          hco_n++;
	        }
	        if (opt_debug) {
	          for(int j=0;j<FIELD_MAX/2;j++) {
	            printf("COEFF: %d %e %e %e %e\n",j,h_co_n[j][0],h_co_n[j][1],h_co_n[j][2],h_co_n[j][3]);
	          }
	        }
	      }
      }
      {
	      struct double_array *pcoef = command_par_array("hcoeffs",cmd->clone);
	      if(pcoef != NULL) {
	        double *hco_s = &h_co_s[0][0];
	        for(int j=0;j<pcoef->curr;j++) {
	          *hco_s = pcoef->a[j];
	          hco_s++;
	        }
	        if (opt_debug) {
	          for(int j=0;j<FIELD_MAX/2;j++) {
	            printf("COEFF: %d %e %e %e %e\n",j,h_co_s[j][0],h_co_s[j][1],h_co_s[j][2],h_co_s[j][3]);
	          }
	        }
	      }
      }
      /* get length of node and check if magnet */
      double ref_str  = 0.0;
      double ref_strn = 0.0;
      double nlength = node_value("l");
      // double ref_len = nlength; // never used
      if (opt_debug)
	      fprintf(prt_file, "original length is %f\n",nlength);
      
      if(strcmp(nextnode->base_name,"multipole") == 0) {
      	double *nvec;
        if ((nvec = mycalloc_atomic("error_efcomp", 1000, sizeof *nvec))) {
	        int lvec;
	        if(rrr > 0 ) {
	          get_node_vector("knl",&lvec,nvec);
	        } else {
	          get_node_vector("ksl",&lvec,nvec);
	        }
	        if (opt_debug) {
	          for(int i=0;i<4;i++) {
	            fprintf(prt_file, "original field = %d is %f\n",i,nvec[i]);
	          }
	        }
          if (n<0) fatal_error("missing or invalid negative order","");
	        if (opt_debug) fprintf(prt_file, "====n====>>> %d %f %f \n\n",n,nvec[n],nlength);
	        ref_str = nvec[n];
	        ref_strn = fabs(ref_str);
	        myfree(rout_name,nvec);
        }
      } else if (strcmp(nextnode->base_name,"sbend") == 0) {
	      double nvec0 = node_value("k0");
	      if (opt_debug) {
	        fprintf(prt_file, "original field0 is %f\n",nvec0);
	        fprintf(prt_file, "====0====>>> %d %f %f \n\n",n,nvec0,nlength);
	      }
	      ref_str = nvec0*nlength;
	      ref_strn = fabs(nvec0);
      } else if (strcmp(nextnode->base_name,"rbend") == 0) {
	      double nvec0 = node_value("k0");
	      if (opt_debug) {
	        fprintf(prt_file, "original field0 is %f\n",nvec0);
	        fprintf(prt_file, "====0====>>> %d %f %f \n\n",n,nvec0,nlength);
	      }
	      ref_str = nvec0*nlength;
	      ref_strn = fabs(nvec0);
      } else if (strcmp(nextnode->base_name,"quadrupole") == 0) {
	      double nvec1 = node_value("k1");
	      if (opt_debug) {
	        fprintf(prt_file, "original field1 is %f\n",nvec1);
	        fprintf(prt_file, "====1====>>> %d %f %f \n\n",n,nvec1,nlength);
	      }
	      ref_str = nvec1*nlength;
	      ref_strn = fabs(nvec1);
      } else if (strcmp(nextnode->base_name,"sextupole") == 0) {
	      double nvec2 = node_value("k2");
	      if (opt_debug) {
	        fprintf(prt_file, "original field2 is %f\n",nvec2);
	        fprintf(prt_file, "====2====>>> %d %f %f \n\n",n,nvec2,nlength);
	      }
	      ref_str = nvec2*nlength;
	      ref_strn = fabs(nvec2);
      } else if (strcmp(nextnode->base_name,"octupole") == 0) {
	      double nvec3 = node_value("k3");
	      if (opt_debug) {
	        fprintf(prt_file, "original field3 is %f\n",nvec3);
	        fprintf(prt_file, "====3====>>> %d %f %f \n\n",n,nvec3,nlength);
	      }
	      ref_str = nvec3*nlength;
	      ref_strn = fabs(nvec3);
      }
      
      /*  edbug print out field components , not done for production version
       */
      
      /* normal components -> 2j, skew components 2j+1 */
      /* AL: the following 'if' is not necessary */
      if(flgmgt == 1) {
	
	      for(unsigned i=0;i<attv_len;i++)  {   /* loop over possible commands */
	        
	        if (opt_debug) fprintf(prt_file, "%s %d\n",attv[i],iattv[i]);
	        
	        struct double_array *ptr = command_par_array(attv[i],cmd->clone);
	        if((ptr != NULL) && (iattv[i] == 1))  { /* command [i] found ? */
	          
	          for(int j=0;j<ptr->curr;j++)  { /* loop over all parameters */
	            
	            /* start field error assignment */
	            /* NORMAL COMPONENTS, ABSOLUTE ERRORS */
	            if(i==0)  {
		            if(add_error_opt == 1) {
		              nextnode->p_fd_err->a[2*j]   += ptr->a[j];
		            } else {
		              nextnode->p_fd_err->a[2*j]    = ptr->a[j];
		            }		
              } else

		      /* SKEW COMPONENTS, ABSOLUTE ERRORS */
              if(i==1) {
		            if(add_error_opt == 1) {
		              nextnode->p_fd_err->a[2*j+1] += ptr->a[j];
		            } else {
		              nextnode->p_fd_err->a[2*j+1]  = ptr->a[j];
		            }
              } else
		
		      /* NORMAL COMPONENTS, RELATIVE ERRORS, MAY BE CORRECTED FOR MEMORY EFFECTS */
              if(i==2) {
		            if(fabs(rr) < 1.0E-6) {
		              printf("++++++ error: trying to assign relative field errors \n");
		              printf("       with no or zero reference radius specified\n");
		              exit(-1);
		            }
		            double norfac = pow(rr,(n-j)) * (fact(j)/fact(n));
		
			          /* if flag for hysteresis correction is set, use coefficients for correction */
      			    double deer = 0.0 ;
      			    if(hyst == 1) {
      			      deer = h_co_n[j][3]*pow(ref_strn,3) + h_co_n[j][2]*pow(ref_strn,2) + h_co_n[j][1]*pow(ref_strn,1) + h_co_n[j][0];
      			      if (opt_debug) 
      				      printf("after correction (n): %d %e %e %e %e\n", j,ref_strn,ptr->a[j],deer,(ptr->a[j] + deer));
      			    }
      			    
      			    if (opt_debug)
      			      fprintf(prt_file, "norm(n): %d %d %f %f\n",n,j,rr,norfac);
      			    
      			    if(add_error_opt == 1) {
      			      nextnode->p_fd_err->a[2*j]   += (ptr->a[j]+deer)*ref_str*norfac;
      			    } else {
      			      nextnode->p_fd_err->a[2*j]    = (ptr->a[j]+deer)*ref_str*norfac;
      			    }
              } else
      			    
  		    /* SKEW COMPONENTS, RELATIVE ERRORS, MAY BE CORRECTED FOR MEMORY EFFECTS */
              if(i==3) {
                if(fabs(rr) < 1.0E-6) {
            			printf("++++++ error: trying to assign relative field errors \n");
            			printf("       with no or zero reference radius specified\n");
            			exit(-1);
      		      }
		            double norfac = pow(rr,(n-j)) * (fact(j)/fact(n));
		
      		      /* if flag for hysteresis correction is set, use coefficients for correction */
      		      double deer = 0.0;
      		      if(hyst == 1) {
            			deer = h_co_s[j][3]*pow(ref_strn,3) + h_co_s[j][2]*pow(ref_strn,2) + h_co_s[j][1]*pow(ref_strn,1) + h_co_s[j][0];
            			if (opt_debug) 
            			  printf("after correction (s): %d %e %e %e %e\n",j,ref_strn,ptr->a[j],deer,(ptr->a[j] + deer));
          		  }
          		      
       		      if (opt_debug)
            			fprintf(prt_file, "norm(s): %d %d %f %f\n",n,j,rr,norfac);
          		      
        	      if(add_error_opt == 1) {
            			nextnode->p_fd_err->a[2*j+1] += (ptr->a[j]+deer)*ref_str*norfac;
                } else {
          	 		  nextnode->p_fd_err->a[2*j+1]  = (ptr->a[j]+deer)*ref_str*norfac;
          		  }
              } else
		      
          /* RF-PHASE OF NORMAL COMPONENTS */
              if(i==4) {
      		      nextnode->p_ph_err->a[2*j] = ptr->a[j];     
              } else

          /* RF-PHASE OF SKEW COMPONENTS */
              if(i==5) {
      		      nextnode->p_ph_err->a[2*j+1] = ptr->a[j];     
		          } /*  end  of field error assignment */ 
	          }
	        }
	      }
      }
    }  /* end of treatment of selected node */
    nextnode = nextnode->next;
  }      /* end of loop over all nodes */
  if(chcount[0] != 0)
    fprintf(prt_file, "Assigned field errors to %d elements\n",chcount[0]);
  if(chcount[1] != 0)
    fprintf(prt_file, "Replaced field errors for %d elements\n",chcount[1]);
  if(chcount[2] != 0)
    fprintf(prt_file, "Added field errors to %d elements\n",chcount[2]);
  //  myfree(rout_name, nvec);
}

static void
error_efield(struct in_cmd* cmd)
{
  int i;
  /*
  struct node *ndexs, *ndexe;
  struct node *nextnode;
  struct name_list* nl = current_error->par_names;
  struct command_parameter_list* pl = current_error->par;
  struct sequence* mysequ = current_sequ;
  */

  fprintf(prt_file, "in efield routine\n");
  fprintf(prt_file, "efield command not yet implemented\n");

  if (get_option("debug"))
  {
   for(i=0;i<cmd->tok_list->curr;i++)
     fprintf(prt_file, "command(s): %s\n",cmd->tok_list->p[i]);
  }
}

static void
error_eoption(struct in_cmd* cmd)
{
  struct name_list* nl = cmd->clone->par_names;
  int i, debug;
  int val, pos, seed;
  int is, ia;
  static  int  ia_seen = 0;

  is = 0; ia = 0;
  
  i = 0;
  while(cmd->tok_list->p[i] != NULL) {
     if(strcmp("add",cmd->tok_list->p[i]) == 0) {
          ia = 1;
     }
     if(strcmp("seed",cmd->tok_list->p[i]) == 0) {
          is = 1;
     }
     i++;
  }
  if ((debug=get_option("debug"))) printf("FOUND: %d %d \n",ia,is);

  if ((debug=get_option("debug")))  {
     fprintf(prt_file, "in eoption routine\n");
     for(i=0;i<cmd->tok_list->curr;i++) {
        fprintf(prt_file, "command(s): %s\n",cmd->tok_list->p[i]);
     }
  }

  if ((pos = name_list_pos("seed", nl)) > -1)
    {
     if (nl->inform[pos])
       {
        seed = command_par_value("seed", cmd->clone);
        init55(seed);
       }
    }

  /* change only if present in command or not yet set */
  if ((ia == 1) || (ia_seen != 1))
  {
       val = command_par_value("add", cmd->clone);
       if(val == 0) {
         if (debug)  fprintf(prt_file, "add option not set\n");
         add_error_opt = 0;
       }else {
         if (debug)  fprintf(prt_file, "add option set\n");
         add_error_opt = 1;
       }
  }


  if(ia == 1) ia_seen = 1;

  if ((debug=get_option("debug"))) printf("err_add eoption: %d seen: %d\n",add_error_opt,ia_seen);

}

// public interface

int
node_al_errors(double* errors)
  /* returns the alignment errors of a node */
{
  if (current_node->p_al_err == NULL) return 0;
  else
  {
    copy_double(current_node->p_al_err->a, errors,
                current_node->p_al_err->curr);
    return current_node->p_al_err->curr;
  }
}

int
node_fd_errors(double* errors)
  /* returns the field errors of a node */
{
  if (current_node->p_fd_err == NULL) return 0;
  else
  {
    copy_double(current_node->p_fd_err->a, errors,
                current_node->p_fd_err->curr);
    return current_node->p_fd_err->curr;
  }
}

int
node_rf_errors(double* errors, double *freq, double *harmon, double *lag )
  /* AL: returns the phase errors of a node */
{
  if (current_node->p_ph_err == NULL) return 0;
  else
  {
    *freq = current_node->rfm_freq;
    *harmon = current_node->rfm_harmon;
    *lag = current_node->rfm_lag;
    copy_double(current_node->p_ph_err->a, errors,
                current_node->p_ph_err->curr);
    return current_node->p_ph_err->curr;
  }
}

void
pro_error(struct in_cmd* cmd)
{
  if (strcmp(cmd->tok_list->p[0], "eoption") == 0) {
    error_eoption(cmd);
    cmd->clone_flag = 1; /* do not drop */
    current_eopt = cmd->clone;
    return;
  }

  if (get_option("debug")) fprintf(prt_file, "enter ERROR module\n");

  if (current_sequ == NULL || current_sequ->ex_start == NULL) {
    warning("ERROR, but no active sequence:", "ignored");
    return;
  }

  if (error_select->curr > 0) set_selected_errors();

       if (strcmp(cmd->tok_list->p[0], "ealign") == 0) error_ealign(cmd);
  else if (strcmp(cmd->tok_list->p[0], "efield") == 0) error_efield(cmd);
  else if (strcmp(cmd->tok_list->p[0], "efcomp") == 0) error_efcomp(cmd);
  else if (strcmp(cmd->tok_list->p[0], "eprint") == 0) error_eprint(cmd);
  else if (strcmp(cmd->tok_list->p[0], "seterr") == 0) error_seterr(cmd);
  else if (strcmp(cmd->tok_list->p[0], "esave" ) == 0) error_esave(cmd);
}

