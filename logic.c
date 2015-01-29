#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<string.h>

#define None -1
#define MAX_CONNECTION 10

/* represent a connecting wire */
struct wire{
	int value;		/* high or low */
	struct gate *owner;	/* which LC owns this wire */
	char *name;		/* name of the wire */
	int activates;			
	int monitor;
	struct wire *connects[MAX_CONNECTION]; /* array of connected wires to the current wire */
	void (*connect)(struct wire *, int, ...);
	void (*set)(struct wire *, int);
};

/* represent a logical gate */
struct gate{
	char *name;
	void (*evaluate)(struct gate *);
	struct NOT *not;
	struct GATE2 *gate2;
	struct XOR *xor;
	struct HA *ha;
	struct FA *fa;
	struct LATCH *latch;
	struct DFF *dff;
	struct DIV2 *div2;
	struct COUNTER *counter;
};

struct NOT{
	struct wire *A;		// in
	struct wire *B;		// out
};

/* used for any 2-in 1-out logic gates */
struct GATE2{
	struct wire *A,*B;	// in
	struct wire *C;		// out
};

struct XOR{
	struct gate *A1,*A2;
	struct gate *I1,*I2;
	struct gate *O1;
};

struct HA{ // half adder
	struct wire *A,*B;	// in
	struct wire *S,*C;	// out
	struct gate *X1;
	struct gate *A1;
};

struct FA{ // full adder
	struct wire *A,*B,*Cin;	// in
	struct wire *S,*Cout;	// out
	struct gate *H1,*H2;
	struct gate *O1;
};

struct LATCH{
	struct wire *A,*B,*Q;
	struct gate *N1,*N2;
};

struct DFF{ // D FlipFlop
	struct wire *D,*C;
	struct wire *Q;
	int prev;
};

struct DIV2{ // Divide by 2 Circuit
	struct wire *C,*D;
	struct wire *Q;
	struct gate *DFF,*NOT;
};

struct COUNTER{ // Counting Register
	struct gate *B0,*B1;
	struct gate *B2,*B3;
};


void testLatch(void);
void testDivBy2(void);
void testCounter(void);
int bit(char *x, int b);
struct gate *Or(char *name);
struct gate *Xor(char *name);
struct gate *And(char *name);
struct gate *Not(char *name);
struct gate *Div2(char *name);
struct gate *Nand(char *name);
struct gate *Latch(char *name);
void test4bit(char *a, char *b);
void eval_or(struct gate *this);
void eval_and(struct gate *this);
struct gate *Counter(char *name);
void eval_not(struct gate *this);
void eval_dff(struct gate *this);
void eval_nand(struct gate *this);
struct gate *HalfAdder(char *name);
struct gate *FullAdder(char *name);
struct gate *DFlipFlop(char *name);
void eval_default(struct gate *this);
void LC(struct gate **thisref, char *name);
void Gate2(struct gate **this, char *name);
void setMethod(struct wire *this, int value);
void connectMethod(struct wire *this, int count, ...);
struct wire *Connector(struct gate *owner, char *name, int activates, int monitor);



/********************************************************************/
/*                        PART ONE                                  */
/********************************************************************/


struct wire *Connector(struct gate *owner, char *name, int activates, int monitor)
{
	struct wire *this = NULL;

	if((this = (struct wire *)malloc(sizeof(struct wire))) == NULL){
		fprintf(stderr,"\nerror : unable to allocate memory for instance\n");
		exit(1);
	}

	if((this->name = strdup(name)) == NULL){
		fprintf(stderr,"\nerror : unable to allocate memory for name\n");
		exit(2);
	}

	this->value = None;
	this->owner = owner;
	this->activates = activates;
	this->monitor = monitor;
	*(this->connects) = NULL;

	this->connect = connectMethod;
	this->set = setMethod;

	return this;
}

void connectMethod(struct wire *this, int count, ...)
{
	int i;
	va_list ap;

	va_start(ap,count);
	for(i = 0; i < count; i++)
		(this->connects)[i] = va_arg(ap, struct wire *);
	(this->connects)[i] = NULL; /* setting last element to NULL */

	va_end(ap);
}

void setMethod(struct wire *this, int value)
{
	int i;

	if(this->value == value) /* Ignore if no change */
		return;

	this->value = value;
	if(this->activates)
		(this->owner->evaluate)(this->owner);
	if(this->monitor)
		printf("Connector %s-%s set to %d\n",this->owner->name,this->name,this->value);

	for(i = 0; (this->connects)[i]; ++i)
		(this->connects)[i]->set((this->connects)[i], value);
}

void LC(struct gate **thisref, char *name)
{
	if((*thisref = (struct gate *)malloc(sizeof(struct gate))) == NULL){
		fprintf(stderr,"\nerror : unable to allocate memory for instance\n");
		exit(1);
	}
	if(((*thisref)->name = strdup(name)) == NULL){
		fprintf(stderr,"\nerror : unable to allocate memory for name\n");
		exit(2);
	}

	(*thisref)->evaluate = eval_default;
}

void eval_default(struct gate *this)
{
	return;
}

struct gate *Not(char *name) /* Inverter. Input A. Output B */
{
	struct gate *this = NULL;

	LC(&this, name);
	if((this->not = (struct NOT *)malloc(sizeof(struct NOT))) == NULL){
		fprintf(stderr,"\nerror : unable to allocate memory for gate->NOT\n");
		exit(3);
	}
	this->not->A = Connector(this,"A",1,0);
	this->not->B = Connector(this,"B",0,0);
	this->evaluate = eval_not;

	return this;
}

void eval_not(struct gate *this)
{
	this->not->B->set(this->not->B, (this->not->A->value == 1) ? 0 : 1);
}

void Gate2(struct gate **thisref, char *name) /* two input gates. Inputs A, B. Output C */
{
	LC(thisref, name);

	struct gate *this = *thisref;
	if((this->gate2 = (struct GATE2 *)malloc(sizeof(struct GATE2))) == NULL){
		fprintf(stderr,"\nerror : unable to allocate memory for gate->GATE2\n");
		exit(3);
	}


	this->gate2->A = Connector(this,"A",1,0);
	this->gate2->B = Connector(this,"B",1,0);
	this->gate2->C = Connector(this,"C",0,0);
}

struct gate *And(char *name)
{
	struct gate *this = NULL;

	Gate2(&this,name);
	this->evaluate = eval_and;

	return this;
}

void eval_and(struct gate *this)
{
	this->gate2->C->set(this->gate2->C, (this->gate2->A->value == 1) && (this->gate2->B->value == 1));
}

struct gate *Or(char *name)
{
	struct gate *this = NULL;

	Gate2(&this,name);
	this->evaluate = eval_or;
}

void eval_or(struct gate *this)
{
	this->gate2->C->set(this->gate2->C, (this->gate2->A->value == 1) || (this->gate2->B->value == 1));
}

struct gate *Xor(char *name)
{
	struct gate *this = NULL;

	Gate2(&this,name);
	if((this->xor = (struct XOR *)malloc(sizeof(struct XOR))) == NULL){
		fprintf(stderr,"\nerror : unable to allocate memory for gate->XOR\n");
		exit(3);
	}

	this->xor->A1 = And("A1");
        this->xor->A2 = And("A2");
        this->xor->I1 = Not("I1");
        this->xor->I2 = Not("I2");
        this->xor->O1 = Or("O1");

        this->gate2->A->connect(this->gate2->A, 2, this->xor->A1->gate2->A, this->xor->I2->not->A);
        this->gate2->B->connect(this->gate2->B, 2, this->xor->I1->not->A, this->xor->A2->gate2->A);

	this->xor->I1->not->B->connect(this->xor->I1->not->B, 1, this->xor->A1->gate2->B);
	this->xor->I2->not->B->connect(this->xor->I2->not->B, 1, this->xor->A2->gate2->B);

	this->xor->A1->gate2->C->connect(this->xor->A1->gate2->C, 1, this->xor->O1->gate2->A);
	this->xor->A2->gate2->C->connect(this->xor->A2->gate2->C, 1, this->xor->O1->gate2->B);

	this->xor->O1->gate2->C->connect(this->xor->O1->gate2->C, 1, this->gate2->C);

	return this;
}

struct gate *HalfAdder(char *name)                       /* One bit adder, A,B in. Sum and Carry out */
{
	struct gate *this = NULL;

        LC(&this, name);
	if((this->ha = (struct HA *)malloc(sizeof(struct HA))) == NULL){
		fprintf(stderr,"\nerror : unable to allocate memory for gate->HA\n");
		exit(3);
	}

        this->ha->A = Connector(this, "A", 1, 0); 
        this->ha->B = Connector(this, "B", 1, 0); 
        this->ha->S = Connector(this, "S", 0, 0);
        this->ha->C = Connector(this,"C", 0, 0);

        this->ha->X1 = Xor("X1");
        this->ha->A1 = And("A1");

        this->ha->A->connect(this->ha->A, 2, this->ha->X1->gate2->A, this->ha->A1->gate2->A);
        this->ha->B->connect(this->ha->B, 2, this->ha->X1->gate2->B, this->ha->A1->gate2->B);

        this->ha->X1->gate2->C->connect(this->ha->X1->gate2->C, 1, this->ha->S);
        this->ha->A1->gate2->C->connect(this->ha->A1->gate2->C, 1, this->ha->C);

	return this;
}

struct gate *FullAdder(char *name)                       /* One bit adder, A,B,Cin in. Sum and Cout out */
{
	struct gate *this = NULL;

	LC(&this,name);
	if((this->fa = (struct FA *)malloc(sizeof(struct FA))) == NULL){
		fprintf(stderr,"\nerror : unable to allocate memory for gate->FA\n");
		exit(3);
	}

        this->fa->A = Connector(this, "A", 1, 1);
        this->fa->B = Connector(this, "B", 1, 1);
        this->fa->Cin = Connector(this, "Cin", 1, 1);
        this->fa->S = Connector(this, "S", 0, 1);
        this->fa->Cout = Connector(this, "Cout", 0, 1);

        this->fa->H1 = HalfAdder("H1");
        this->fa->H2 = HalfAdder("H2");
        this->fa->O1 = Or("O1");

        this->fa->A->connect(this->fa->A, 1, this->fa->H1->ha->A );
        this->fa->B->connect(this->fa->B, 1, this->fa->H1->ha->B );
        this->fa->Cin->connect(this->fa->Cin, 1, this->fa->H2->ha->A );

        this->fa->H1->ha->S->connect(this->fa->H1->ha->S, 1, this->fa->H2->ha->B );
        this->fa->H1->ha->C->connect(this->fa->H1->ha->C, 1, this->fa->O1->gate2->B);
        this->fa->H2->ha->C->connect(this->fa->H2->ha->C, 1, this->fa->O1->gate2->A);
        this->fa->H2->ha->S->connect(this->fa->H2->ha->S, 1, this->fa->S);
        this->fa->O1->gate2->C->connect(this->fa->O1->gate2->C, 1, this->fa->Cout);

	return this;
}

int bit(char *x, int b)
{
	return (x[b] == '1') ? 1 : 0;
}

void test4bit(char *a, char *b) /* a, b four char strings like '0110' */
{
	struct gate *F0,*F1,*F2,*F3;

	F0 = FullAdder("F0");
	F1 = FullAdder("F1");
	F0->fa->Cout->connect(F0->fa->Cout, 1, F1->fa->Cin);
	F2 = FullAdder("F2");
	F1->fa->Cout->connect(F1->fa->Cout, 1, F2->fa->Cin);
	F3 = FullAdder("F3");
	F2->fa->Cout->connect(F2->fa->Cout, 1, F3->fa->Cin);

	F0->fa->Cin->set(F0->fa->Cin, 0);
	F0->fa->A->set(F0->fa->A, bit(a, 3));
	F0->fa->B->set(F0->fa->B, bit(b, 3));			/* bits in lists are reversed from natural order */
	F1->fa->A->set(F1->fa->A, bit(a, 2));
	F1->fa->B->set(F1->fa->B, bit(b, 2));
	F2->fa->A->set(F2->fa->A, bit(a, 1));
	F2->fa->B->set(F2->fa->B, bit(b, 1));
	F3->fa->A->set(F3->fa->A, bit(a, 0));
	F3->fa->B->set(F3->fa->B, bit(b, 0));

	printf("\n%d %d %d %d %d\n",F3->fa->Cout->value,F3->fa->S->value,F2->fa->S->value,F1->fa->S->value,F0->fa->S->value);
}



/********************************************************************/
/*                        PART TWO                                  */
/********************************************************************/


struct gate *Nand(char *name) /* two input NAND Gate */
{
	struct gate *this = NULL;

	Gate2(&this,name);
	this->evaluate = eval_nand;

	return this;
}

void eval_nand(struct gate *this)
{
	this->gate2->C->set(this->gate2->C, !((this->gate2->A->value == 1) && (this->gate2->B->value == 1)));
}

struct gate *Latch(char *name)
{
	struct gate *this = NULL;

	LC(&this,name);
	if((this->latch = (struct LATCH *)malloc(sizeof(struct LATCH))) == NULL){
		fprintf(stderr,"\nerror : unable to allocate memory for gate->LATCH\n");
		exit(3);
	}

	this->latch->A = Connector(this,"A",1,0);
	this->latch->B = Connector(this,"B",1,0);
	this->latch->Q = Connector(this,"Q",0,1);

	this->latch->N1 = Nand("N1");	
	this->latch->N2 = Nand("N2");

	this->latch->A->connect(this->latch->A, 1, this->latch->N1->gate2->A);
	this->latch->B->connect(this->latch->B, 1, this->latch->N2->gate2->B);
	this->latch->N1->gate2->C->connect(this->latch->N1->gate2->C, 2, this->latch->N2->gate2->A, this->latch->Q);
	this->latch->N2->gate2->C->connect(this->latch->N2->gate2->C, 1, this->latch->N1->gate2->B);

	return this;
}

void testLatch(void)
{
	char ans[5];
	struct gate *x = Latch("ff1");

	x->latch->A->set(x->latch->A,1);
	x->latch->B->set(x->latch->B,1);

	while("True"){
		printf("\nInput A or B to drop : ");
		fgets(ans,sizeof(ans),stdin);
		ans[strlen(ans)-1] = '\0'; // removing '\n' before '\0' 

		if((strcmp(ans,"A") == 0) || (strcmp(ans,"a") == 0)){
			x->latch->A->set(x->latch->A,0);
			x->latch->A->set(x->latch->A,1);
		}
		else if((strcmp(ans,"B") == 0) || (strcmp(ans,"b") == 0)){
			x->latch->B->set(x->latch->B,0);
			x->latch->B->set(x->latch->B,1);
		}
		else
			break;
	}
	printf("\n\n");
}

struct gate *DFlipFlop(char *name)
{
	struct gate *this = NULL;

	LC(&this,name);
	if((this->dff = (struct DFF *)malloc(sizeof(struct DFF))) == NULL){
		fprintf(stderr,"\nerror : unable to allocate memory for gate->DFF\n");
		exit(3);
	}

	this->dff->D = Connector(this, "D", 1, 0);
	this->dff->C = Connector(this, "C", 1, 0);
	this->dff->Q = Connector(this, "Q", 0, 0);

	this->dff->Q->value = 0;
	this->dff->prev = None;

	this->evaluate = eval_dff;

	return this;
}

void eval_dff(struct gate *this)
{
	if((this->dff->C->value == 0) && (this->dff->prev == 1)) /* Clock drop */
		this->dff->Q->set(this->dff->Q,this->dff->D->value);
	this->dff->prev = this->dff->C->value;
}

struct gate *Div2(char *name)
{
	struct gate *this = NULL;

	LC(&this,name);
	if((this->div2 = (struct DIV2 *)malloc(sizeof(struct DIV2))) == NULL){
		fprintf(stderr,"\nerror : unable to allocate memory for gate->DFF\n");
		exit(3);
	}

	this->div2->C = Connector(this, "C", 1, 0);
	this->div2->D = Connector(this, "D", 0, 0);
	this->div2->Q = Connector(this, "Q", 0, 1);

	this->div2->Q->value = 0;

	this->div2->DFF = DFlipFlop("DFF");
	this->div2->NOT = Not("NOT");

	this->div2->C->connect(this->div2->C, 1, this->div2->DFF->dff->C);
	this->div2->D->connect(this->div2->D, 1, this->div2->DFF->dff->D);

	this->div2->DFF->dff->Q->connect(this->div2->DFF->dff->Q, 2, this->div2->NOT->not->A, this->div2->Q);
	this->div2->NOT->not->B->connect(this->div2->NOT->not->B, 1, this->div2->DFF->dff->D);

	this->div2->DFF->dff->Q->activates = 1;
	this->div2->DFF->dff->D->value = 1 - this->div2->DFF->dff->Q->value;

	return this;
}

void testDivBy2(void)
{
	int c;
	int ch;
	struct gate *x;

	c = 0;
	x = Div2("X");
	x->div2->C->set(x->div2->C,c);

	while(1){
		printf("Clock is %d. Hit return to toggle clock ",c);
		if((ch = getchar()) == EOF)
			break;
		c = !c;
		x->div2->C->set(x->div2->C,c);
	}
	printf("\n\n");
}

struct gate *Counter(char *name)
{
	struct gate *this = NULL;

	LC(&this,name);
	if((this->counter = (struct COUNTER *)malloc(sizeof(struct COUNTER))) == NULL){
		fprintf(stderr,"\nerror : unable to allocate memory for gate->DFF\n");
		exit(3);
	}

	this->counter->B0 = Div2("B0");
	this->counter->B1 = Div2("B1");
	this->counter->B2 = Div2("B2");
	this->counter->B3 = Div2("B3");

	this->counter->B0->div2->Q->connect(this->counter->B0->div2->Q, 1, this->counter->B1->div2->C);
	this->counter->B1->div2->Q->connect(this->counter->B1->div2->Q, 1, this->counter->B2->div2->C);
	this->counter->B2->div2->Q->connect(this->counter->B2->div2->Q, 1, this->counter->B3->div2->C);

	return this;
}

void testCounter(void)
{
	int ch;
	struct gate *x = Counter("x"); /* x is a four bit counter */

	x->counter->B0->div2->C->set(x->counter->B0->div2->C,1); /* set the clock line 1 */
	while(1){
		printf("Count is %d %d ",x->counter->B3->div2->Q->value,x->counter->B2->div2->Q->value);
		printf("%d %d",x->counter->B1->div2->Q->value,x->counter->B0->div2->Q->value);

		printf("\nHit return to pulse the clock ");
		if((ch = getchar()) == EOF)
			break;
		
		x->counter->B0->div2->C->set(x->counter->B0->div2->C,0); /* toggle the clock */
		x->counter->B0->div2->C->set(x->counter->B0->div2->C,1);
	}
	printf("\n\n");
}

main()
{
	printf("\nNot output\n");
	struct gate *sn = Not("N1");
	sn->not->B->monitor = 1;
	sn->not->A->set(sn->not->A,0);
	sn->not->A->set(sn->not->A,1);
	printf("\n\n");

	printf("\nAnd output\n");
	struct gate *sa = And("A1");
	sa->gate2->C->monitor = 1;
	sa->gate2->A->set(sa->gate2->A,1);
	sa->gate2->B->set(sa->gate2->B,1);
	sa->gate2->A->set(sa->gate2->A,0);
	printf("\n\n");

	printf("\nOr output\n");
	struct gate *o = Or("O1");
	o->gate2->C->monitor = 1;
	o->gate2->A->set(o->gate2->A,0);
	o->gate2->B->set(o->gate2->B,1);
	printf("\n\n");

	printf("\nNand output : connecting AND to NOT\n");
	struct gate *a = And("A1");
	struct gate *n = Not("N1");
	a->gate2->C->monitor = 1;
	a->gate2->C->connect(a->gate2->C,1,n->not->A);
	n->not->B->monitor = 1;
	a->gate2->B->set(a->gate2->B,1);
	a->gate2->A->set(a->gate2->A,1);
	printf("\n\n");

	printf("\nXor output\n");
	struct gate *x = Xor("X1");
	x->gate2->C->monitor = 1;
	x->gate2->A->set(x->gate2->A,0);
	x->gate2->B->set(x->gate2->B,1);
	x->gate2->A->set(x->gate2->A,1);
	x->gate2->B->set(x->gate2->B,0);
	printf("\n\n");

	printf("\nHalf Adder output\n");
	struct gate *h1 = HalfAdder("H1");
	h1->ha->S->monitor=1;
	h1->ha->C->monitor=1;
	h1->ha->A->set(h1->ha->A,0);
	h1->ha->B->set(h1->ha->B,0);
	h1->ha->B->set(h1->ha->B,1);
	h1->ha->A->set(h1->ha->A,1);

	test4bit("1110", "0010");

	printf("\nNand output\n");
	struct gate *na = Nand("A1");
	na->gate2->C->monitor = 1;
	na->gate2->A->set(na->gate2->A,1);
	na->gate2->B->set(na->gate2->B,1);
	na->gate2->A->set(na->gate2->A,0);
	printf("\n\n");

	printf("\nLatch output\n");
	testLatch();

	printf("\nDivBy2 output\n");
	testDivBy2();

	printf("\nCounter output\n");
	testCounter();
}
