/*
*
*   DRIVER SCRIPT TO RUN THE ENTIRE PROJECT
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../headers/fem.h"
#include "../headers/glfem.h"

#define TRUE 1
#define FALSE 0
typedef int bool;


int main(int argc, char* argv[]) {

    // paths to subfolder main functions if needed
    const char* meshFilePath = "data/mesh.txt";
    const char* rawMeshFilePath = "data/mesh_raw.txt";
    const char* fixedMeshFilePath = "data/mesh_fixed.txt";
    const char* nodeDisplacementsFilePath = "data/nodal_displacements.txt";

    // runtime argument parser
    bool carabiner_open = FALSE;
    double mesh_size = 0.5;
    double vertical_force = 5e6;
    double deformation_factor = 1e0;
    bool aluminium = TRUE;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--o") == 0) carabiner_open = TRUE;
        if (strcmp(argv[i], "--rough") == 0) mesh_size = 1.0;
        if (strcmp(argv[i], "--medium") == 0) mesh_size = 0.4;
        if (strcmp(argv[i], "--fine") == 0) mesh_size = 0.2;
        if (strcmp(argv[i], "--tiny") == 0) mesh_size = 0.15;
        if (strcmp(argv[i], "--fstrong") == 0) vertical_force = 5e9;
        if (strcmp(argv[i], "--fweak") == 0) vertical_force = 5e3;
        if (strcmp(argv[i], "--steel") == 0) aluminium = FALSE;
        if (strcmp(argv[i], "--amplify") == 0) deformation_factor = 1e3;
        if (strcmp(argv[i], "--help") == 0) { /* help(); */ exit(0); }
    }

    printf("Running parameters:\n");
    printf("\tCarabiner is %s", (carabiner_open)? "OPEN" : "CLOSED");
    printf("\tMesh size: %f \n", mesh_size);
    printf("\tVertical force: %f \n", vertical_force);
    printf("\tDeformation factor: %f \n", deformation_factor);
    printf("\tMaterial: %s", (aluminium)? "Aluminium" : "Steel");

    //
    // PREPROCESSING
    //

    geoInitialize();
    femGeo *theGeometry = geoGetGeometry();
    theGeometry->h = mesh_size;
    theGeometry->elementType = FEM_TRIANGLE;

    if (carabiner_open == TRUE) geoMeshGenerateOpen();
    else geoMeshGenerateClosed();
        
    geoMeshImport();
    printf("\n>> Raw mesh summary:\n");
    printf("\tGlobal Mesh size: %f\n", theGeometry->h);
    printf("\tNumber of raw nodes: %d", theGeometry->theNodes->nNodes);
    printf("\tNumber of domains: %d\n", theGeometry->nDomains);

    geoMeshWrite(rawMeshFilePath);
    printf("\n>> Cleaning up raw mesh data...\n");
    system(".venv/bin/python src/fixmesh.py data/mesh_raw.txt data/mesh_fixed.txt");
    printf(">> Done cleaning mesh. New connected mesh at data/mesh_fixed.txt\n");

    geoMeshRead(fixedMeshFilePath);
    printf("\n>> theGeometry updated with fixed mesh\n");

    //
    // DEFINING THE PROBLEM
    //

    double E = (aluminium)? 68e9 : 211e9;
    double nu = (aluminium)? 0.32 : 0.30;
    double rho = (aluminium)? 2.71e3 : 7.85e3;
    double g = -9.81;

    femProblem *theProblem = femElasticityCreate(theGeometry, E, nu, rho, g, PLANAR_STRESS);
    printf("\n>> theProblem created\n");
    
    int numberOfDomains = theProblem->geometry->nDomains;
    for (int iDom = 0; iDom < numberOfDomains; iDom++) {
        if (carabiner_open == FALSE && iDom == 12) {
            geoSetDomainName(iDom, "Bottom Contact Surface");
            femElasticityAddBoundaryCondition(theProblem, "Bottom Contact Surface", DIRICHLET_Y, 0.0);
        }
        if (carabiner_open == FALSE && iDom == 13) {
            geoSetDomainName(iDom, "Top Contact Surface");
            femElasticityAddBoundaryCondition(theProblem, "Top Contact Surface", NEUMANN_Y, vertical_force);
        }
        if (carabiner_open == TRUE && iDom == 8) {
            geoSetDomainName(iDom, "Top Contact Surface");
            femElasticityAddBoundaryCondition(theProblem, "Top Contact Surface", NEUMANN_Y, vertical_force);
        }
        if (carabiner_open == TRUE && iDom == 12) {
            geoSetDomainName(iDom, "Bottom Contact Surface");
            femElasticityAddBoundaryCondition(theProblem, "Bottom Contact Surface", DIRICHLET_Y, 0.0);
        }
    }

    femElasticityPrint(theProblem);

    //
    // SOLVING
    //

    int nNodes = theGeometry->theNodes->nNodes;
    printf(">> Solving elasticity problem...\n");
    double *theSoluce = femElasticitySolve(theProblem);
    printf(">> Solving for forces...\n");
    double *theForces = femElasticityForces(theProblem);
    double area = femElasticityIntegrate(theProblem, fun);

    femSolutionWrite(nNodes, 2, theSoluce, nodeDisplacementsFilePath);

    //
    // POSTPROCESSING
    //

    femNodes *theNodes = theGeometry->theNodes;
    double *normDisplacement = malloc(theNodes->nNodes * sizeof(double));
    double *forcesX = malloc(theNodes->nNodes * sizeof(double));
    double *forcesY = malloc(theNodes->nNodes * sizeof(double));

    for (int i=0; i<theNodes->nNodes; i++){
        theNodes->X[i] += theSoluce[2*i]*deformation_factor;
        theNodes->Y[i] += theSoluce[2*i+1]*deformation_factor;
        normDisplacement[i] = sqrt(theSoluce[2*i]*theSoluce[2*i] + theSoluce[2*i+1]*theSoluce[2*i+1]);
        forcesX[i] = theForces[2*i];
        forcesY[i] = theForces[2*i+1];
    }

    printf(" ==== Deformation Factor            : %14.7e \n",deformation_factor);
    printf(" ==== Minimum displacement          : %14.7e [m] \n",femMin(normDisplacement,nNodes));
    printf(" ==== Maximum displacement          : %14.7e [m] \n",femMax(normDisplacement,nNodes));

    double theGlobalForce[2] = {0, 0};
    for (int i=0; i<nNodes; i++) {
        theGlobalForce[0] += theForces[2*i];
        theGlobalForce[1] += theForces[2*i+1]; }
    printf(" ==== Global horizontal force       : %14.7e [N] \n",theGlobalForce[0]);
    printf(" ==== Global vertical force         : %14.7e [N] \n",theGlobalForce[1]);
    printf(" ==== Weight                        : %14.7e [N] \n", area * 0.01 * rho * g);

    int mode = 1, domain = 0, freezingButton = FALSE;
    double t, told = 0;
    char theMessage[MAXNAME];
   
    GLFWwindow* window = glfemInit("EPL1110 : Project 2024-25 : Deformation of a Carabiner Under Longitudinal Load");
    glfwMakeContextCurrent(window);

    do {
        int w,h;
        glfwGetFramebufferSize(window,&w,&h);
        glfemReshapeWindows(theGeometry->theNodes,w,h);

        t = glfwGetTime();  
        if (glfwGetKey(window,'D') == GLFW_PRESS) mode = 0;
        if (glfwGetKey(window,'V') == GLFW_PRESS) mode = 1;
        if (glfwGetKey(window,'X') == GLFW_PRESS) mode = 2;
        if (glfwGetKey(window,'Y') == GLFW_PRESS) mode = 3;
        if (glfwGetKey(window,'N') == GLFW_PRESS && freezingButton == FALSE) {
            domain++; freezingButton = TRUE; told = t; }
        if (t - told > 0.5) freezingButton = FALSE;

        if (mode == 0) {
            domain = domain % theGeometry->nDomains;
            glfemPlotDomain(theGeometry->theDomains[domain]); 
            sprintf(theMessage, "%s : %d ", theGeometry->theDomains[domain]->name, domain);
            glColor3f(1.0,0.0,0.0); glfemMessage(theMessage);
        }
        if (mode == 1) {
            glfemPlotField(theGeometry->theElements, normDisplacement);
            glfemPlotMesh(theGeometry->theElements); 
            sprintf(theMessage, "Number of elements : %d ", theGeometry->theElements->nElem);
            glColor3f(1.0,0.0,0.0); glfemMessage(theMessage);
            double uMin = femMin(normDisplacement, theNodes->nNodes);
            double uMax = femMax(normDisplacement, theNodes->nNodes);
            glfemDrawColorBar(uMin, uMax);

        }
        if (mode == 2) {
            glfemPlotField(theGeometry->theElements, forcesX);
            glfemPlotMesh(theGeometry->theElements); 
            sprintf(theMessage, "Number of elements : %d ", theGeometry->theElements->nElem);
            glColor3f(1.0,0.0,0.0); glfemMessage(theMessage);
        }
        if (mode == 3) {
            glfemPlotField(theGeometry->theElements, forcesY);
            glfemPlotMesh(theGeometry->theElements); 
            sprintf(theMessage, "Number of elements : %d ", theGeometry->theElements->nElem);
            glColor3f(1.0,0.0,0.0); glfemMessage(theMessage);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();

    } while( glfwGetKey(window,GLFW_KEY_ESCAPE) != GLFW_PRESS &&
             glfwWindowShouldClose(window) != 1 );

    free(normDisplacement); free(forcesX); free(forcesY);
    femElasticityFree(theProblem); 
    geoFinalize(); glfwTerminate(); 
    exit(EXIT_SUCCESS);
    return 0;
}

void help(void) {
    printf("Acceptable flags: \n");
    printf("\tGeometry options:\n");
    printf("\t\t--o : simulates an opened carabiner\n");
    printf("\t\tDefault is closed\n");
    printf("\tMesh options:\n");
    printf("\t\t--rough : sets global mesh size to 1.0\n");
    printf("\t\t--medium : sets global mesh size to 0.4\n");
    printf("\t\t--fine : sets global mesh size to 0.25\n");
    printf("\t\t--tiny : sets global mesh size to 0.15\n");
    printf("\t\tDefault is 0.5\n");
    printf("\tForce options:\n");
    printf("\t\t--fstrong : sets load to 5e9\n");
    printf("\t\t--fweak : sets load to 5e3\n");
    printf("\t\tDefault is 5e6\n");
    printf("\tMaterial options:\n");
    printf("\t\t--steel : sets material to steel\n");
    printf("\t\tDefault is aluminium\n");
    printf("\tVisualisation options:\n");
    printf("\t\t--amplify : sets displacement amplification factor to 1e3\n");
    printf("\t\tDefault is 1\n");
}
