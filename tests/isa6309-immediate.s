; test immediate 6309 instructions

	adcd	#$1089
	adcr	u,x
	adde	#$8b
	addf	#$cb
	addr	u,d
	addw	#$108b
	andd	#$1084
	andr	u,s

	bitd	#$1085
	bitmd	#$3c

	cmpe	#$81
	cmpf	#$c1
	cmpr	u,v
	cmpw	#$1081

	divd	#$8d
	divq	#$118e

	eord	#$1088
	eorr	u,w

	lde	#$86
	ldf	#$c6
	ldmd	#$3d
	ldq	#$cdcdcdcd
	ldw	#$1086

	muld	#$118f

	ord	#$108a
	orr	u,pc

	sbcd	#$1082
	sbcr	u,u
	sube	#$80
	subf	#$c0
	subr	u,y
	subw	#$1080

	tfm	d+,x+
	tfm	y-,u-
	tfm	s+,u
	tfm	y,x+
