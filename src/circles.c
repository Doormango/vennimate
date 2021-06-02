/**
 * @file    circles.c
 * @brief   Animation exhibiting arrangements of 4 circles in affine plane.
 *
 * Implementation of functions to animate between arrangements of 4 circles in
 * the affine plane.
 *
 * @author  D. Trinchero (dario.trinchero@pm.me)
 * @date    2020-05-31
 */

#include <stdio.h>
#include <GL/glut.h>
#include <math.h>
#include "circles.h"
#include "random.h"

/* --- user-set parameters -------------------------------------------------- */

#define CIRCLE_PTS		200
#define SCALE			200.0f
#define ALPHA			0.27f
#define FRAMERATE		60
#define MULTISAMPLING	16

double animDuration = 2.7;
int nonLinCtl = 12; // bounds: NON_LIN_CTL_LO, NON_LIN_CTL_HI

/* --- constants ------------------------------------------------------------ */

#define NON_LIN_CTL_HI		21
#define NON_LIN_CTL_LO		-1
#define DURATION_CTL_LO		0.2
#define DURATION_CTL_DELTA	0.05

/* precomputed in main */
static unsigned int refreshMillis;
static unsigned int animFrames;
static double nonLin;

/* --- animation trackers --------------------------------------------------- */

static unsigned int currGroupIdx = 0;
static unsigned int currAnimFrame = 0;

/* --- initialization routines ---------------------------------------------- */

void circInit(void) {
	unsigned int i, j, k, tmp;
	int idx;
	double avCoord;

	// subtract average group position
	for (i = 0 ; i < NUM_GROUPS; i++) {
		for (k = 0; k < 2; k++) {
			avCoord = 0;
			for (j = 0; j < 4; j++) avCoord += circleGroups[i][j][k];
			for (j = 0; j < 4; j++) circleGroups[i][j][k] -= avCoord / 4.0;
		}
	}
	
	// initialize & Fisher-Yates shuffle circle group order
	for (i = 0; i < NUM_GROUPS; i++) groupOrder[i] = i;
	for (i = 0; i < NUM_GROUPS - 1; i++) {
		idx = rnd_int32(i, NUM_GROUPS);
		tmp = groupOrder[idx];
		groupOrder[idx] = groupOrder[i];
		groupOrder[i] = tmp;
	}
}

void animInit(void) {
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_ALPHA | GLUT_MULTISAMPLE);

	glutCreateWindow("Circles");
	glutFullScreen();
	glutSetCursor(GLUT_CURSOR_NONE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_ONE); // (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)?

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glEnable(GL_MULTISAMPLE);
	glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
	glutSetOption(GLUT_MULTISAMPLE, MULTISAMPLING);

	glDisable(GL_DEPTH_TEST);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glLineWidth(2.0);
}

/* --- animation routines --------------------------------------------------- */

void drawCircle(double cx, double cy, double r) {
	unsigned int i;
	double theta = 2 * M_PI / CIRCLE_PTS;
	double c = cos(theta), s = sin(theta);
	double x = r, y = 0;
	double t;

	glBegin(GL_TRIANGLE_FAN);
	glColor4f(1, 1, 1, ALPHA);
	for (i = 0; i < CIRCLE_PTS; i++) {
		glVertex2f(x + cx, y + cy);
		// apply rotation matrix
		t = x;
		x = c * x - s * y;
		y = s * t + c * y;
	}
	glEnd();
}

double animEase(double t, double nonLin) {
	if (t < 0.5) return 0.5 * pow(2 * t, nonLin);
	else return 1 - 0.5 * pow(2 * (1 - t), nonLin);
}

void drawInterpCurve(double x, double y, double width, double height, unsigned int samples) {
	unsigned int i;
	double t;

	glBegin(GL_LINE_STRIP);
	glColor3f(1, 1, 1);
	for (i = 0; i < samples; i++) {
		t = i / (double) samples;
		glVertex2f(x + width * t, y + height * animEase(t, nonLin));
	}
	glEnd();
}

void animDisplay(void) {
	double interpolateStep;
	unsigned int nextGroupIdx;
	double (*currGroup)[3], (*nextGroup)[3];
	unsigned int j;

	glClear(GL_COLOR_BUFFER_BIT);

	interpolateStep = animEase(currAnimFrame / (double) animFrames, nonLin);
	nextGroupIdx = (currGroupIdx + 1) % NUM_GROUPS;
	currGroup = circleGroups[groupOrder[currGroupIdx]];
	nextGroup = circleGroups[groupOrder[nextGroupIdx]];

	for (j = 0; j < 4; j++) {
		drawCircle(
			currGroup[j][0] * (1 - interpolateStep) + nextGroup[j][0] * interpolateStep,
			currGroup[j][1] * (1 - interpolateStep) + nextGroup[j][1] * interpolateStep,
			currGroup[j][2] * (1 - interpolateStep) + nextGroup[j][2] * interpolateStep
		);
	}

	drawInterpCurve(0, 0, 80, 40, 100);

	currAnimFrame = (currAnimFrame + 1) % animFrames;
	if (!currAnimFrame) currGroupIdx = nextGroupIdx;

	glutSwapBuffers(); // double-buffering for smoother animation
}

void animReshape(GLsizei width, GLsizei height) {
	GLfloat aspect;

	if (!height) height = 1;
	aspect = (GLfloat) width / (GLfloat) height;

	glViewport(0, 0, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (width >= height) {
		gluOrtho2D(-aspect * SCALE, aspect * SCALE, -SCALE, SCALE);
	} else {
		gluOrtho2D(-SCALE, SCALE, -SCALE / aspect, SCALE / aspect);
	}
}

void animKeyboard(unsigned char key, int x, int y) {
	switch (key) {
		case 27: // esc
			exit(EXIT_SUCCESS);
			break;
		default:
			break;
	}
}

double nonLinCtlCurve(int nonLinCtl) {
	if (nonLinCtl == -1) return 1;
	else return pow(2, nonLinCtl / 2.0 - 2) + 1;
}

void animSpecialKeys(int key, int x, int y) {
	unsigned int oldAnimFrames;

	switch (key) {
		case GLUT_KEY_RIGHT:
			if (animDuration > DURATION_CTL_LO) {
				animDuration -= DURATION_CTL_DELTA;
				oldAnimFrames = animFrames;
				animFrames = animDuration * FRAMERATE;
				currAnimFrame = currAnimFrame / (double) oldAnimFrames * animFrames;
			}
			break;
		case GLUT_KEY_LEFT:
			animDuration += DURATION_CTL_DELTA;
			oldAnimFrames = animFrames;
			animFrames = animDuration * FRAMERATE;
			currAnimFrame = currAnimFrame / (double) oldAnimFrames * animFrames;
			break;
		case GLUT_KEY_UP:
			if (nonLinCtl < NON_LIN_CTL_HI) {
				nonLin = nonLinCtlCurve(++nonLinCtl);
			}
			break;
		case GLUT_KEY_DOWN:
			if (nonLinCtl > NON_LIN_CTL_LO) {
				nonLin = nonLinCtlCurve(--nonLinCtl);
			}
			break;
		default:
			break;
	}
}

void animTimer(int value) {
	glutPostRedisplay();
	glutTimerFunc(refreshMillis, animTimer, 0);
}

/* --- main method ---------------------------------------------------------- */

int main(int argc, char *argv[]) {
	// calculate precomputed constants
	refreshMillis = 1000 / FRAMERATE;
	animFrames = animDuration * FRAMERATE;
	nonLin = nonLinCtlCurve(nonLinCtl);

	// run initialization functions
	putenv((char *) "__GL_SYNC_TO_VBLANK=1");
	glutInit(&argc, argv);
	rnd_init();
	circInit();
	animInit();

	// register functions & enter main loop
	glutReshapeFunc(animReshape);
	glutKeyboardFunc(animKeyboard);
	glutSpecialFunc(animSpecialKeys);
	glutDisplayFunc(animDisplay);
	glutTimerFunc(0, animTimer, 0);
	glutMainLoop();
	return EXIT_SUCCESS;
}
