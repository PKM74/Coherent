/*
 * make -- maintain program groups
 * td 80.09.17
 * things done:
 *	20-Oct-82	Made nextc properly translate "\\\n[ 	]*" to ' '.
 *	15-Jan-85	Made portable enough for z-8000, readable enough for
 *			human beings.
 *	06-Nov-85	Added free(t) to make() to free used space.
 *	07-Nov-85	Modified docmd() to call varexp() only if 'cmd'
 *			actually contains macros, for efficiency.
 *	24-Feb-86	Minor fixes by rec to many things.  Deferred
 *			macro expansion in actions until needed, deferred
 *			getmdate() until needed, added canonicalization in
 *			archive searches, allowed ${NAME} in actions for
 *			shell to expand, put macro definitions in malloc,
 *			entered environ into macros.
 */

#include	"make.h"

char usage[] = "Usage: make [-isrntqpd] [-f file] [macro=value] [target]";
char nospace[] = "Out of space";
char badmac[] = "Bad macro name";
char incomp[] = "Incomplete line at end of file";

int iflag;			/* ignore command errors */
int sflag;			/* don't print command lines */
int rflag;			/* don't read built-in rules */
int nflag;			/* don't execute commands */
int tflag;			/* touch files rather than run commands */
int qflag;			/* zero exit status if file up to date */
int pflag;			/* print macro defns, target descriptions */
int dflag;			/* debug mode -- verbose printout */
int eflag;			/* make environ macros protected */


FILE *fd;
int defining = 0;		/* nonzero => do not expand macros */
time_t now;
unsigned char backup[NBACKUP];
int nbackup = 0;
int lastc;
int lineno;
char macroname[NMACRONAME+1];
struct token{
	struct token *next;
	char *value;
};
char *token;
char tokbuf[NTOKBUF];
int toklen;
char *tokp;
struct macro {
	struct macro *next;
	char *value;
	char *name;
	int protected;
};
struct macro *macro;
char *deftarget = NULL;
struct sym{
	struct sym *next;
	char *action;
	char *name;
	struct dep *deplist;
	int type;
	time_t moddate;
};
struct sym *sym = NULL;
struct sym *suffixes;
struct sym *deflt;

struct dep{
	struct dep *next;
	char *action;
	struct sym *symbol;
};

struct stat statbuf;
char	*mvarval[4];		/* list of macro variable values */

/* Interesting function declarations */

char *mexists();
char *extend();
char *mmalloc();
struct token *listtoken();
struct sym *sexists();
struct sym *dexists();
struct sym *lookup();
struct dep *adddep();

/* cry and die */
die(str)
char	*str;
{
	fprintf(stderr, "make: %r\n", &str);
	exit(ERROR);
}
/* print lineno, cry and die */
err(s)
char *s;
{
	fprintf(stderr, "make: %d: %r\n", lineno, &s);
	exit(ERROR);
}

/* Malloc nbytes and abort on failure */
char *mmalloc(n) int n;
{
	char *p;
	if (p = malloc(n))
		return p;
	err(nospace);
}

/* read characters from backup (where macros have been expanded) or from
 * whatever the open file of the moment is. keep track of line #s.
 */

readc()
{
	if(nbackup!=0)
		return(backup[--nbackup]);
	if(lastc=='\n')
		lineno++;
	lastc=getc(fd);
	return(lastc);
}

/* put c into backup[] */

putback(c)
{
	if(c==EOF)
		return;
	if (nbackup == NBACKUP)
		err("Macro definition too long");
	backup[nbackup++]=c;
}

/* put s into backup */

unreads(s)
register char *s;
{
	register char *t;

	t = &s[strlen(s)];
	while (t > s)
		putback(*--t);
}

/* return a pointer to the macro definition assigned to macro name s.
 * return NULL if the macro has not been defined.
 */

char *mexists(s) register char *s;
{
	register struct macro *i;

	for (i = macro; i != NULL; i=i->next)
		if (Streq(s, i->name))
			return (i->value);
	return (NULL);
}

/* install macro with name name and value value in macro[]. Overwrite an
 * existing value if it is not protected.
 */

define(name, value, protected)
register char *name, *value;
{
	register struct macro *i;

	if(dflag)
		printf("define %s = %s\n", name, value);
	for (i = macro; i != NULL; i=i->next)
		if (Streq(name, i->name)) {
			if (!i->protected) {
				free(i->value);
				i->value = value;
				i->protected = protected;
			} else if (dflag)
				printf("... definition suppressed\n");
			return;
		}
	i = (struct macro *)mmalloc(sizeof(*i));
	i->name = name;
	i->value = value;
	i->protected = protected;
	i->next = macro;
	macro = i;
}

/* Accept single letter user defined macros */
ismacro(c) register int c;
{
	if ((c>='0'&&c<='9')
	 || (c>='a'&&c<='z')
	 || (c>='A'&&c<='Z'))
		return 1;
	return 0;
}
/* return the next character from the input file. eat comments, return EOS
 * for a newline not followed by an action, \n for newlines that are followed
 * by actions;  if not in a macro definition or action specification
 * then expand the macro in backup or complain about the name.
 */

nextc()
{
	register char *s;
	register c;

Again:
	if((c=readc())=='#'){
		do
			c=readc();
		while(c!='\n' && c!=EOF);
	}
	if(c=='\n'){
		if((c=readc())!=' ' && c!='\t'){
			putback(c);
			return(EOS);
		}
		do
			c=readc();
		while(c==' ' || c=='\t');
		putback(c);
		return('\n');
	}
	if(c=='\\'){
		c=readc();
		if(c=='\n') {
			while ((c=readc())==' ' || c=='\t')
				;
			putback(c);
			return(' ');
		}
		putback(c);
		return('\\');
	}
	if(!defining && c=='$'){
		c=readc();
		if(c=='(') {
			s=macroname;
			while(' '<(c=readc()) && c<0177 && c!=')')
				if(s!=&macroname[NMACRONAME])
					*s++=c;
			if (c != ')')
				err(badmac);
			*s++ = '\0';
		} else if (ismacro(c)) {
			macroname[0]=c;
			macroname[1]='\0';
		} else
			err(badmac);
		if((s=mexists(macroname))!=NULL)
			unreads(s);
		goto Again;
	}
	return(c);
}

/* Get a block of l0+l1 bytes copy s0 and s1 into it, and return a pointer to
 * the beginning of the block.
 */

char *
extend(s0, l0, s1, l1)
char *s0, *s1;
{
	register char *t;

 	if (s0 == NULL)
 		t = mmalloc(l1);
 	else {
 		if ((t = realloc(s0, l0 + l1)) == NULL)
			err(nospace);
	}
	strncpy(t+l0, s1, l1);
	return(t);
}

/* Return 1 if c is EOS, EOF, or one of the characters in s */

delim(c, s)
register char	c;
char	*s;
{
	return (c == EOS || c == EOF || index(s, c) != NULL);
}

/* Prepare to copy a new token string into the token buffer; if the old value
 * in token wasn't saved, tough matzohs.
 */

starttoken()
{
	token=NULL;
	tokp=tokbuf;
	toklen=0;
}

/* Put c in the token buffer; if the buffer is full, copy its contents into
 * token and start agin at the beginning of the buffer.
 */

addtoken(c)
{
	if(tokp==&tokbuf[NTOKBUF]){
		token=extend(token, toklen-NTOKBUF, tokbuf, NTOKBUF);
		tokp=tokbuf;
	}
	*tokp++=c;
	toklen++;
}

/* mark the end of the token in the buffer and save it in token. */

endtoken()
{
	addtoken('\0');
	token=extend(token, toklen-(tokp-tokbuf), tokbuf, tokp-tokbuf);
}

/* Install value at the end of the token list which begins with next; return
 * a pointer to the beginning of the list, which is the one just installed if
 * next was NULL.
 */

struct token *
listtoken(value, next)
char *value;
struct token *next;
{
	register struct token *p;
	register struct token *t;

	t=(struct token *)mmalloc(sizeof *t);	/*Necessaire ou le contraire?*/
	t->value=value;
	t->next=NULL;
	if(next==NULL)
		return(t);
	for(p=next;p->next!=NULL;p=p->next);
	p->next=t;
	return(next);
}

/* Free the overhead of a token list */
struct token *freetoken(t) register struct token *t;
{
	register struct token *tp;
	while (t != NULL) {
		tp = t->next;
		free(t);
		t = tp;
	}
	return t;
}

/* Read macros, dependencies, and actions from the file with name file, or
 * from whatever file is already open. The first string of tokens is saved
 * in a list pointed to by tp; if it was a macro, the definition goes in 
 * token, and we install it in macro[]; if tp points to a string of targets,
 * its depedencies go in a list pointed to by dp, and the action to recreate
 * it in token, and the whole shmear is installed.
 */

input(file)
char *file;
{
	struct token *tp = NULL, *dp = NULL;
	register c;
	char *action;
	int twocolons;

	if(file!=NULL && (fd=fopen(file, "r"))==NULL)
		die("can't open %s", file);
	lineno=1;
	lastc=EOF;
	for(;;){
		c=nextc();
		for(;;){
			while(c==' ' || c=='\t')
				c=nextc();
			if(delim(c, "=:;\n"))
				break;
			starttoken();
			while(!delim(c, " \t\n=:;")){
				addtoken(c);
				c=nextc();
			}
			endtoken();
			tp=listtoken(token, tp);
		}
		switch(c){
		case EOF:
			if(tp!=NULL)
				err(incomp);
			fclose(fd);
			return;
		case EOS:
			if(tp==NULL)
				break;
		case '\n':
			err("Newline after target or macroname");
		case ';':
			err("; after target or macroname");
		case '=':
			if(tp==NULL || tp->next!=NULL)
				err("= without macro name or in token list");
			defining++;
			while((c=nextc())==' ' || c=='\t');
			starttoken();
			while(c!=EOS && c!=EOF) {
				addtoken(c);
				c=nextc();
			}
			endtoken();
			define(tp->value, token, 0);
			defining=0;
			break;
		case ':':
			if(tp==NULL)
				err(": without preceding target");
			c=nextc();
			if(c==':'){
				twocolons=1;
				c=nextc();
			} else
				twocolons=0;
			for(;;){
				while(c==' ' || c=='\t')
					c=nextc();
				if(delim(c, "=:;\n"))
					break;
				starttoken();
				while(!delim(c, " \t\n=:;")){
					addtoken(c);
					c=nextc();
				}
				endtoken();
				dp=listtoken(token, dp);
			}
			switch(c){
			case ':':
				err("::: or : in or after dependency list");
			case '=':
				err("= in or after dependency");
			case EOF:
				err(incomp);
			case ';':
			case '\n':
				++defining;
				starttoken();
				while((c=nextc())!=EOS && c!=EOF)
					addtoken(c);
				endtoken();
				defining = 0;
				action=token;
				break;
			case EOS:
				action=NULL;
			}
			install(tp, dp, action, twocolons);
		}
		tp = freetoken(tp);
		dp = freetoken(dp);
		dp = NULL;
	}
}

/* Input with library lookup */
inlib(file) char *file;
{
	register char *p, *cp;
	if ((p = getenv("LIBPATH")) == NULL)
		p = DEFLIBPATH;
	cp = path(p, file, AREAD);
	input(cp ? cp : file);
}

/* Input first file in list which is found via path */
inpath(file) char *file;
{
	register char **vp, *p, *cp;
	if ((p = getenv("PATH")) == NULL)
		p = DEFPATH;
	for (vp = &file; *vp != NULL; vp += 1)
		if ((cp = path(p, *vp, AREAD)) != NULL)
			break;
	input(cp ? cp : file);
}

/* Return the last modified date of file with name name. If it's an archive,
 * open it up and read the insertion date of the pertinent member.
 */

time_t
getmdate(name)
char	*name;
{
	char	*subname;
	char	*lwa;
	int	fd;
	int	magic;
	time_t	result;
	struct ar_hdr	hdrbuf;

	if (stat(name, &statbuf) ==0)
		return(statbuf.st_mtime);
	subname = index(name, '(');
	if (subname == NULL)
		return (0);
	lwa = &name[strlen(name) - 1];
	if (*lwa != ')')
		return (0);
	*subname = NUL;
	fd = open(name, READ);
	*subname++ = '(';
	if (fd == EOF)
		return (0);
	if (read(fd, &magic, sizeof magic) != sizeof magic) {
		close(fd);
		return (0);
	}
	canint(magic);
	if (magic != ARMAG) {
		close(fd);
		return (0);
	}
	*lwa = NUL;
	result = 0;
	while (read(fd, &hdrbuf, sizeof hdrbuf) == sizeof hdrbuf) {
		if (strcmp(hdrbuf.ar_name, subname) == 0) {
			cantime(hdrbuf.ar_date);
			result = hdrbuf.ar_date;
			break;
		}
		canlong(hdrbuf.ar_size);
		lseek(fd, hdrbuf.ar_size, REL);
	}
	*lwa = ')';
	return (result);
}


/* Does file name exist? */

fexists(name)
char *name;
{
	return(stat(name, &statbuf)>=0);
}

/* Return a pointer to the symbol table entry with name "name", NULL if it's
 * not there.
 */

struct sym *sexists(name) register char *name;
{
	register struct sym *sp;

	for(sp=sym;sp!=NULL;sp=sp->next)
		if(Streq(name, sp->name))
			return(sp);
	return(NULL);
}

/*
 * Return a pointer to the member of deplist which has name as the last
 * part of it's pathname, otherwise return NULL.
 */
struct sym *dexists(name, dp) register char *name; register struct dep *dp;
{
	register char *p;
	while (dp != NULL) {
		if ((p = rindex(dp->symbol->name, PATHSEP)) && Streq(name, p+1))
			return dp->symbol;
		else
			dp = dp->next;
	}
	return NULL;
}
/* Look for symbol with name "name" in the symbol table; install it if it's
 * not there; initialize the action and dependency lists to NULL, the type to
 * unknown, zero the modification date, and return a pointer to the entry.
 */

struct sym *
lookup(name)
char *name;
{
	register struct sym *sp;

	if((sp=sexists(name))!=NULL)
		return(sp);
	sp = (struct sym *)mmalloc(sizeof (*sp));	/*necessary?*/
	sp->name=name;
	sp->action=NULL;
	sp->deplist=NULL;
	sp->type=T_UNKNOWN;
	sp->moddate=0;
	sp->next=sym;
	sym=sp;
	return(sp);
}

/* Install a dependency with symbol having name "name", action "action" in 
 * the end of the dependency list pointed to by next. If s has already 
 * been noted as a file in the dependency list, install action. Return a 
 * pointer to the beginning of the dependency list.
 */

struct dep *
adddep(name, action, next)
char *name, *action;
struct dep *next;
{
	register struct dep *v;
	register struct sym *s;
	struct dep *dp;

	s=lookup(name);
	for(v=next;v!=NULL;v=v->next)
		if(s==v->symbol){
			if (action != NULL) {
				if(v->action!=NULL)
					err("Multiple detailed actions for %s",
						s->name);
				v->action=action;
			}
			return(next);
		}
	v = (struct dep *)malloc(sizeof (*v));	/*necessary?*/
	v->symbol=s;
	v->action=action;
	v->next=NULL;
	if(next==NULL)
		return(v);
	for(dp=next;dp->next!=NULL;dp=dp->next);
	dp->next=v;
	return(next);
}

/* Do everything for a dependency with left-hand side cons, r.h.s. ante, 
 * action "action", and one or two colons. If cons is the first target in the
 * file, it becomes the default target. Mark each target in cons as detailed
 * if twocolons, undetailed if not, and install action in the symbol table 
 * action slot for cons in the latter case. Call adddep() to actually create
 * the dependency list.
 */

install(cons, ante, action, twocolons)
struct token *ante, *cons;
char *action;
{
	struct sym *cp;
	struct token *ap;

	if(deftarget==NULL && cons->value[0]!='.')
		deftarget=cons->value;
	if(dflag){
		printf("Ante:");
		ap=ante;
		while(ap!=NULL){
			printf(" %s", ap->value);
			ap=ap->next;
		}
		printf("\nCons:");
		ap=cons;
		while(ap!=NULL){
			printf(" %s", ap->value);
			ap=ap->next;
		}
		printf("\n");
		if(action!=NULL)
			printf("Action: '%s'\n", action);
		if(twocolons)
			printf("two colons\n");
	}
	for (; cons != NULL; cons = cons->next) {
		cp=lookup(cons->value);
		if(cp==suffixes && ante==NULL)
			cp->deplist=NULL;
		else{
			if(twocolons){
				if(cp->type==T_UNKNOWN)
					cp->type=T_DETAIL;
				else if(cp->type!=T_DETAIL)
					err("'::' not allowed for %s",
						cp->name);
			} else {
				if(cp->type==T_UNKNOWN)
					cp->type=T_NODETAIL;
				else if(cp->type!=T_NODETAIL)
					err("Must use '::' for %s", cp->name);
				if (action != NULL) {
					if(cp->action != NULL)
						err("Multiple actions for %s",
							cp->name);
					cp->action = action;
				}
			}
			for(ap=ante;ap!=NULL;ap=ap->next)
				cp->deplist=adddep(ap->value,
					twocolons?action:NULL, cp->deplist);
		}
	}
}

/* Make s; first, make everything s depends on; if the target has detailed
 * actions, execute any implicit actions associated with it, then execute
 * the actions associated with the dependencies which are newer than s. 
 * Otherwise, put the dependencies that are newer than s in token ($?), 
 * make s if it doesn't exist, and call docmd.
 */

make(s)
register struct sym *s;
{
	register struct dep *dep;
	register char *t;
	int update;
	int type;

	if(s->type==T_DONE)
		return;
	if(dflag)
		printf("Making %s\n", s->name);
	type=s->type;
	s->type=T_DONE;
	s->moddate=getmdate(s->name);
	for(dep=s->deplist;dep!=NULL;dep=dep->next)
		make(dep->symbol);
	if(type==T_DETAIL){
		implicit(s, "", 0);
		for(dep=s->deplist;dep!=NULL;dep=dep->next)
			if(dep->symbol->moddate>s->moddate)
				docmd(s, dep->action, s->name,
					dep->symbol->name, "", "");
	} else {
		update=0;
		starttoken();
		for(dep=s->deplist;dep!=NULL;dep=dep->next){
			if(dflag)
				printf("%s time=%ld %s time=%ld\n",
				    dep->symbol->name, dep->symbol->moddate,
				    s->name, s->moddate);
			if(dep->symbol->moddate>s->moddate){
				update++;
				addtoken(' ');
				for(t=dep->symbol->name;*t;t++)
					addtoken(*t);
			}
		}
		endtoken();
		t = token;
		if (!update && !fexists(s->name)) {
			update = TRUE;
			if (dflag)
				printf("'%s' made due to non-existence\n",
					s->name);
		}
		if(s->action==NULL)
			implicit(s, t, update);
		else if(update)
			docmd(s, s->action, s->name, t, "", "");
		free(t);
	}
}

/*
 * Expand substitutes the macros in actions and returns the string.
 */
expand(str) register char *str;
{
	register int c;
	register char *p;
	while (c = *str++) {
		if (c == '$') {
			c = *str++;
			switch (c) {
			case 0: err(badmac);
			case '$': addtoken(c); continue;
			case '{': addtoken('$'); addtoken(c); continue;
			case '@': p = mvarval[0]; break;
			case '?': p = mvarval[1]; break;
			case '<': p = mvarval[2]; break;
			case '*': p = mvarval[3]; break;
			case '(':
				p = str;
				do c = *str++; while (c!=0 && c!=')');
				if (c == 0)
					err(badmac);
				*--str = 0;
				p = mexists(p);
				*str++ = ')';
				break;
			default:
				if ( ! ismacro(c))
					err(badmac);
				c = *str;
				*str = 0;
				p = mexists(str-1);
				*str = c;
				break;
			}
			if (p != NULL)
				expand(p);
		} else
			addtoken(c);
	}
}

/* Mark s as modified; if tflag, touch s, otherwise execute the necessary
 * commands.
 */
docmd(s, cmd, at, ques, less, star)
struct sym *s;
char *cmd, *at, *ques, *less, *star;
{
	static char	touch[] = "touch $@";

	if (dflag)
		printf("ex '%s'\n\t$@='%s'\n\t$?='%s'\n\t$<='%s'\n\t$*='%s'\n",
			cmd, at, ques, less, star);
	if (qflag)
		exit(NOTUTD);
	s->moddate = now;
	if (tflag)
		cmd = touch;
	if (cmd == NULL)
		return;
	mvarval[0] = at;
	mvarval[1] = ques;
	mvarval[2] = less;
	mvarval[3] = star;
	starttoken();
	expand(cmd);
	endtoken();
	doit(token);
	free(token);
}


/* look for '-' (ignore errors) and '@' (silent) in cmd, then execute it
 * and note the return status.
 */

doit(cmd)
register char	*cmd;
{
	register char *mark;
	int sflg, iflg, rstat;

	if (nflag) {
		printf("%s\n", cmd);
		return;
	}
	do {
		mark = index(cmd, '\n');
		if (mark != NULL)
			*mark = NUL;
		if (*cmd == '-') {
			++cmd;
			iflg = TRUE;
		} else
			iflg = iflag;
		if (*cmd == '@') {
			++cmd;
			sflg = TRUE;
		} else
			sflg = sflag;
		if (!sflg)
			printf("%s\n", cmd);
		fflush(stdout);
		rstat = system(cmd);
		if (rstat != 0 && !iflg)
			if (sflg)
				die("%s	exited with status %d",
					cmd, rstat);
			else
				die("	exited with status %d", rstat);
		cmd = mark + 1;
	} while (mark != NULL && *cmd != NUL);
}


/* Find the implicit rule to generate obj and execute it. Put the name of 
 * obj up to '.' in prefix, and look for the rest in the dependency list 
 * of .SUFFIXES. Find the file "prefix.foo" upon which obj depends, where
 * foo appears in the dependency list of suffixes after the suffix of obj.
 * Then make obj according to the rule from makeactions. If we can't find 
 * any rules, use .DEFAULT, provided we're definite.
 */

implicit(obj, ques, definite)
struct sym *obj;
char *ques;
{
	register char *s;
	register struct dep *d;
	char *prefix, *file, *rulename, *suffix;
	struct sym *rule;
	struct sym *subj;

	if(dflag)
		printf("Implicit %s (%s)\n", obj->name, ques);
	if ((suffix=rindex(obj->name, '.')) == NULL
	 || suffix==obj->name) {
		if (definite)
			defalt(obj, ques);
		return;
	}
	starttoken();
	for(s=obj->name; s<suffix; s++)
		addtoken(*s);
	endtoken();
	prefix=token;
	for(d=suffixes->deplist;d!=NULL;d=d->next)
		if(Streq(suffix, d->symbol->name))
			break;
	if(d==NULL){
		free(prefix);
		if (definite)
			defalt(obj, ques);
		return;
	}
	while((d=d->next)!=NULL){
		starttoken();
		for(s=obj->name; s!=suffix; s++)
			addtoken(*s);
		for(s=d->symbol->name;*s;s++)
			addtoken(*s);
		endtoken();
		file=token;
		subj=NULL;
		if(fexists(file) || (subj=dexists(file, obj->deplist))){
			starttoken();
			for(s=d->symbol->name;*s!='\0';s++)
				addtoken(*s);
			for(s=suffix;*s!='\0';s++)
				addtoken(*s);
			endtoken();
			rulename=token;
			if((rule=sexists(rulename))!=NULL){
				if (subj != NULL || (subj=sexists(file))) {
					free(file);
					file=subj->name;
				} else
					subj=lookup(file);
				make(subj);
				if(definite || subj->moddate>obj->moddate)
					docmd(obj, rule->action,
						obj->name, ques, file, prefix);
				free(prefix);
				free(rulename);
				return;
			}
			free(rulename);
		}
		free(file);
	}
	free(prefix);
	if (definite)
		defalt(obj, ques);
}


/*
 * Deflt uses the commands associated to '.DEFAULT' to make the object
 * 'obj'.
 */

defalt(obj, ques)
struct sym	*obj;
char		*ques;
{
	if (deflt == NULL)
		die("Don't know how to make %s", obj->name);
	docmd(obj, deflt->action, obj->name, ques, "", "");
}


main(argc, argv, envp) char *argv[], *envp[];
{
	register char	*s, *value;
	register int c;
	struct token	*fp = NULL;
	struct sym	*sp;
	struct dep	*d;
	struct macro	*mp;

	time(&now);
	++argv;
	--argc;
	while (argc > 0 && argv[0][0] == '-')
		for (--argc, s = *argv++; *++s != NUL;)
			switch (*s) {
			case 'i': iflag++; break;
			case 's': sflag++; break;
			case 'r': rflag++; break;
			case 'n': nflag++; break;
			case 't': tflag++; break;
			case 'q': qflag++; break;
			case 'p': pflag++; break;
			case 'd': dflag++; break;
			case 'e': eflag++; break;
			case 'f':
				if (--argc < 0)
					Usage();
				fp=listtoken(*argv++, fp);
				break;
			default:
				Usage();
			}
	while (argc > 0 && (value = index(*argv, '=')) != NULL) {
		s = *argv;
		while (*s != ' ' && *s != '\t' && *s != '=')
			++s;
		*s = '\0';
		define(*argv++, value+1, 1);
		--argc;
	}
	while (*envp != NULL) {
		if ((value = index(*envp, '=')) != NULL
		 && index(value, '$') == NULL) {
			s = *envp;
			while ((c=*s) != ' ' && c != '\t' && c != '=')
				++s;
			*s = 0;
			if (eflag)
				define(*envp, value+1, 1);
			else {
				starttoken();
				while (*++value) addtoken(*value);
				endtoken();
				define(*envp, token, 0);
			}
			*s = c;
		}
		++envp;
	}
	suffixes=lookup(".SUFFIXES");
	if (!rflag)
		inlib(MACROFILE);
	deftarget = NULL;
	if (fp == NULL)
		inpath("makefile", "Makefile", NULL);
	else {
		fd = stdin;
		do {
			input( strcmp(fp->value, "-") == 0 ? NULL : fp->value);
			fp = fp->next;
		} while (fp != NULL);
	}
	if (!rflag)
		inlib(ACTIONFILE);
	if (sexists(".IGNORE") != NULL)
		++iflag;
	if (sexists(".SILENT") != NULL)
		++sflag;
	deflt = sexists(".DEFAULT");
	if(pflag){
		if(macro != NULL) {
			printf("Macros:\n");
			for (mp = macro; mp != NULL; mp=mp->next)
				printf("%s=%s\n", mp->name, mp->value);
		}
		printf("Rules:\n");
		for(sp=sym;sp!=NULL;sp=sp->next){
			if(sp->type!=T_UNKNOWN){
				printf("%s:", sp->name);
				if(sp->type==T_DETAIL)
					putchar(':');
				for(d=sp->deplist;d!=NULL;d=d->next)
					printf(" %s", d->symbol->name);
				printf("\n");
				if(sp->action)
					printf("\t%s\n", sp->action);
			}
		}
	}
	if(argc > 0){
		do{
			make(lookup(*argv++));
		} while (--argc > 0);
	} else
		make(lookup(deftarget));
	exit(ALLOK);
}

/* Whine about usage and then quit */

Usage()
{
	fprintf(stderr, "%s\n", usage);
	exit(1);
}

