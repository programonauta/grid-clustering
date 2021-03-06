#include <iostream>
#include <math.h>
#include <unistd.h>
#include "cell.h"
#include "csv-reader.h"
#include "graphs.h"

bool isDouble(const char *str)
{
  char* endptr = 0;
  strtod(str, &endptr);

  return !((endptr == str));
}

int main(int argc, char* argv[])
{
    ifstream infile;
    ofstream svgFile;
    int opt;
    string input = "../data/input.csv";
    string outputNodes = "nodes.txt";
    string outputEdges = "edges.txt";
    string outputSVG = "graph.svg";
    string outputDT = "decision.txt";
    string value;
    int lineNo = 0;
    int dimension;
    int noFields;  // number of fields
    vector<string> lineCSV, headerCSV, clusterFields;
    vector<Cell*> listCells; // Vector of Cells
    vector<Edge> listEdges; // Vector of Edges
    vector<Cell*> roots;    // Vector of cluster's roots
    vector<double> maxValue, minValue;
    long qtyCellsVal = 0;

    FILE* pFileNodes;
    FILE* pFileEdges;
    FILE* pFileDT;

    int epsilon = 10; // Epsilon: number of divisions
    int minPoints = 3; // Minimum number of points

    cout << "Grid Clustering algorithm" << endl;
    cout << "developed by ricardo brandao - https://github.com/programonauta/grid-clustering" << endl;
    cout << "===============================================================================\n" << endl;

    // Initialize draw options
    bool drawPoints = true;
    bool drawRects = true;
    bool drawEdges = true;

    // Default values of arguments
    epsilon = 10;
    input = "../data/input.txt";
    outputNodes = "nodes.txt";
    outputEdges = "edges.txt";

    // Verify command line options
    //
    // Show help message if -h or there is no command options

    while ((opt = getopt(argc, argv, "e:m:i:n:g:prah")) != -1)
    {
        switch (opt)
        {
        case 'e':
            epsilon = atoi(optarg);
            if (epsilon < 1)
                epsilon = 10;
            break;
        case 'm':
            minPoints = atoi(optarg);
            if (minPoints < 1)
                minPoints = 3;
            break;
        case 'i':
            input = optarg;
            break;
        case 'n':
            outputNodes = optarg;
            break;
        case 'g':
            outputEdges = optarg;
            break;
        case 'p':
            drawPoints = false;
            break;
        case 'r':
            drawRects = false;
            break;
        case 'a':
            drawEdges = false;
            break;
        case 'h':
        default: /* '?' */
            cout << "OVERVIEW: Clustering data using grid-based algorithm" << endl;
            cout << "          The program output node and edges file in a format " <<
              "to be imported by Gephi software" << endl;
            cout << endl;
            cout << "USAGE: gcluster <options>" << endl;
            cout << endl;
            cout << "OPTIONS: " << endl;
            cout << "-e <value>\tvalue of epsilon (default: 10)" << endl;
            cout << "-m <value>\tMinimum Points (default: 3)" << endl;
            cout << "-i <file>\tinput file (default: ../data/input.csv)" << endl;
            cout << "-n <file>\tnodes output file (default: nodes.txt)" << endl;
            cout << "-g <file>\tedge output file (default: output.txt)" << endl;
            cout << "-p don't draw points" << endl;
            cout << "-r don't draw rectangles" << endl;
            cout << "-a don't draw edges" << endl;
            exit(EXIT_FAILURE);
        }
    }

    cout << "Parameters" << endl;
    cout << "----------" << endl;
    cout << "epsilon: " << epsilon << endl;
    cout << "Min Points: " << minPoints << endl;
    cout << "Input file: " << input << endl;
    cout << "Nodes file: " << outputNodes << endl;
    cout << "Edges file: " << outputEdges << endl;
    cout << endl <<"Running" << endl;
    cout << "-------" << endl;

  infile.open(input.c_str());
  if (!infile)
  {
    cout << "Error: Unable to open input."<< endl;
    return 1;
  }

  pFileNodes = fopen(outputNodes.c_str(), "w");

  if (pFileNodes == NULL)
  {
    cout << "Error: Unable do open nodes output file." << endl;
    infile.close();
    return 1;
  }

  pFileEdges = fopen(outputEdges.c_str(), "w");

  if (pFileEdges == NULL)
  {
    cout << "Error: Unable do open edges output file." << endl;
    infile.close();
    fclose(pFileNodes);
    return 1;
  }

  int r = 0;

  if (!getCSVLine(infile, headerCSV))
  {
    cout << "Error reader CSV File (Header) on the first line" << endl;
    return 1;
  }

  noFields = headerCSV.size(); // Define number of fields

  cout << "Reading CSV File header. Number of Fields:" << noFields << endl;

  for (int i = 0; i < headerCSV.size(); i++)
    cout << (i?",": "") << headerCSV[i];

  cout << endl;

  if (!getCSVLine(infile, clusterFields))
  {
    cout << "Error reading CSV File (Header) on Clusters Line" << endl;
    return 1;
  }

  if (clusterFields.size() != noFields)
  {
    cout << "Error reading CSV File (Header): Clusters Line (line 2)- "
      << "number of fields doesn't "
      << "match. Read " << clusterFields.size() << " Expected: " << noFields
      << " fields " << endl;
    return 1;
  }

  dimension = 0;
  for (int i = 0; i < clusterFields.size(); i++)
  {
    if (clusterFields[i] != "C" && clusterFields[i] != "N")
    {
      cout << "Error reading CSV File (Header): Clusters Line (line 2)"
        << "- Invalid Parameter"
        << " on field " << i+1 << " \"" << clusterFields[i] << "\""
        << " Must be \"C\" (C)luster Field " <<
        " or \"N\" (N)ot a Cluster Field " << endl;
      return 1;
    }
    if (clusterFields[i] == "C")
      dimension++;
  }

  if (dimension == 0)
  {
    cout << "Error reading CSV File (Header): Cluster Line (line 2) "
      << "- There is not clusters fields "
      << endl;
    return 1;
  }

  cout << "CSV File - Dimension: " << dimension << endl;

  cout << "Fields used on clustering: ";

  for (int i = 0; i < clusterFields.size(); i++)
  {
    if (clusterFields[i] == "C")
      cout << headerCSV[i] << " ";
  }

  cout << endl;

  if (!getCSVLine(infile, lineCSV))
  {
    cout << "Error reading CSV File (Header) on Max line (line 3)" << endl;
    return 1;
  }

  if (lineCSV.size() != noFields)  // Test if line of max values has the same number of fields than header
  {
    cout << "Error reading CSV File (Header): Max line (line 3)"
      << "- number of fields doesn't match. "
      << " Read " << lineCSV.size() << " Expected: " << noFields <<  endl ;
    return 1;
  }

  maxValue.resize(noFields);
  minValue.resize(noFields);

  for (int i = 0; i < noFields; i++)
  {
    if (!isDouble(lineCSV[i].c_str()))
    {
      cout << "Error reading CSV File (Header): Max line (line 3)"
        << " Field number " << i+1 << ", "
        << lineCSV[i] << " is not double" << endl;
      return 1;
    }
    maxValue[i] = stod(lineCSV[i]);
  }

  cout << "CSV File - Max Line read - OK" << endl;
  lineCSV.resize(0);
  lineCSV.shrink_to_fit();

  if (!getCSVLine(infile, lineCSV))
  {
    cout << "Error reading CSV File (Header) on Min line (line 4)" << endl;
    return 1;
  }

  if (lineCSV.size() != noFields) // Test if line of min values has the same dimension than header
  {
    cout << "Error reading CSV File (Header): Min line (line 4) - number of fields "
      << "doesn't match. Read " << lineCSV.size() << " Expected: " << noFields << endl ;
    return 1;
  }

  for (int i = 0; i < noFields; i++)
  {
    if (!isDouble(lineCSV[i].c_str()))
    {
      cout << "Error reading CSV File (Header): Min line (line 4)"
        << " Field number " << i+1 << ", "
        << lineCSV[i] << " is not double" << endl;
      return 1;
    }
    minValue[i] = stod(lineCSV[i]);

    if (minValue[i] >= maxValue[i]) // Test if min value is greater or equal than max value
    {
      cout << "Error reading CSV File (Header) - line 4: Min value gretaer or equal"
        << "than Max value for dimension "
        << i+1 << ": " << minValue[i] << " isn't less than " << maxValue[i] << endl;
      return 1;
    }
  }

  cout << "CSV File - Min Line read - OK" << endl;
  lineCSV.resize(0);
  lineCSV.shrink_to_fit();

  cout << "CSV File - Reading Data" << endl;

  // Start read data (5th line)

  vector <Point> pointsSample;

  while (infile)
  {
    r = getCSVLine(infile, lineCSV);
    ++lineNo;
    if (!r)
    {
      cout << "Error reading CSV File (Data) on Line " << lineNo + 4  << "\n";
      return 1;
    }
    if (lineCSV.size() != noFields)
    {
      cout << "Error reading CSV File (Data) on Line " << lineNo + 4 << ": number of "
        << "fields doesn't match" << endl;
      return 1;
    }

    Point sample(dimension);

    int posSample = 0;

    for (int i = 0; i < lineCSV.size(); i++)
    {
      if (clusterFields[i] != "C")
        continue;
      if (!isDouble(lineCSV[i].c_str()))
      {
        cout << "Error reading CSV File (Data) on Line: " << lineNo + 4 << ", Field: "
          << i+1 << " \""
          << lineCSV[i] << "\" is not double" << endl;
        return 1;
      }
      // Update the coordinates with normalized values
      sample.coord[posSample++] =
        ((stod(lineCSV[i])-minValue[i])/(maxValue[i]-minValue[i]));
    }

    pointsSample.push_back(sample);

    // Here we know the CSV line is correct, so let's create a sample normalized

    Cell* pNewCell;

    vector<int> coordCell; // Cells coordinates

    coordCell.resize(dimension);

    pNewCell = NULL;

    // Determine the cell's coordinates

    for (int i = 0; i < dimension; i++)
      coordCell[i] = (int)(sample.coord[i] * epsilon);

    for (int i=0; i<listCells.size(); i++) // Search cells
    {
      // Compare listCells and current cell
      if (coordCell == listCells[i]->coord) // all coordinates are equal
      {
        pNewCell = listCells[i];
        break;
      }
    }

    if (pNewCell == NULL) // Create a New Cell
    {
      pNewCell = new Cell(dimension);
      for (int i=0; i < dimension; i++)
        pNewCell->coord[i] = coordCell[i];
      listCells.push_back(pNewCell);
    }

    if (!pNewCell->insertPoint(sample))
    {
      cout << "Error on insert Point on Cell - Line " << lineNo << endl;
      return 1;
    }

    cout << flush <<  "Number of Lines: " << lineNo << " Number of Cells: " << listCells.size() << "\r";

    lineCSV.resize(0);
  }

  Point a(dimension);

  cout << "CSV File - Total of " << lineNo << " lines of data read" << endl;
  cout << "Size of each point: " << sizeof(a.coord[0]) << endl;
  cout << "Size of vector: " << sizeof(a.coord) << " dimension: " << dimension <<  endl;
  cout << "Number of Cells: " << listCells.size() << endl;
  cout << "Size of each cell: " << sizeof(*listCells[0]);
  cout << "Size of Cells: " << sizeof(*listCells[0])*listCells.size() <<
    " Size of Points: " << sizeof(a) * lineNo << endl;


  if (listCells.size() == 0)
  {
    cout << "Error on algorithm: There is no cells" << endl;
    return 1;
  }

  cout << "------------------------------" << endl;
  cout << "Creating Nodes and Edges files" << endl;
  cout << "------------------------------" << endl ;

  roots.push_back(listCells[0]);

  // -----------------------
  // nodes.txt file
  // -----------------------

  // Create header of nodes.txt file

  fprintf(pFileNodes, "id,Label,Number_Points");

  // Labels from coordinates
  for (int i=0; i < dimension; i++)
    fprintf(pFileNodes, ",Coord-%d", i);

  // Labels of dimensions
  for (int i=0; i < headerCSV.size(); i++)
  {
    if (clusterFields[i]=="C")
      fprintf(pFileNodes, ",%s", headerCSV[i].c_str());
  }

  fprintf(pFileNodes, "\n");

  // -------------------------
  // edges.txt file
  // -------------------------

  // Create edge header

  fprintf(pFileEdges, "Source,Target,Type,Id,Label,Weight\n");

  int labelEdge = 0;

  // Create a graph: one node for each cell

  Graph g(listCells.size());

  // iterate all cells
  for (int i = 0; i<listCells.size(); i++)
  {
    fprintf(pFileNodes, "%d,%d", i, i);

    // Number of points
    fprintf(pFileNodes, ",%ld", listCells[i]->getQtyPoints());

    // Cell's coordinates
    for (int j = 0; j < listCells[i]->coord.size(); j++)
      fprintf(pFileNodes, ",%d", listCells[i]->coord[j]);

    Point CM = listCells[i]->getCenterMass(); // Center of mass
    // Center of mass coordinates
    for (int j = 0; j < listCells[i]->coord.size(); j++)
      fprintf(pFileNodes, ",%f", CM.coord[j]);

    fprintf(pFileNodes, "\n");

    // if we are at last cell, there is not a next cell, so we can exit the loop

    if (i==listCells.size()-1)
      break;

    // Verify the quantity of points
    if (listCells[i]->getQtyPoints() < minPoints)
      continue;

    qtyCellsVal++;
    // test next cell until end

    for (int j = i+1; j<listCells.size(); j++)
    {
      // Verify the quantity of points
      if (listCells[j]->getQtyPoints() < minPoints)
        continue;

      if (areAdjacents(listCells[i], listCells[j]))
      {
        // Create an edge on Graph

        g.addEdge(i, j, 1.0);

        bool gravitational = false;
        long qtdPointsI = listCells[i]->getQtyPoints();
        long qtdPointsJ = listCells[j]->getQtyPoints();
        fprintf(pFileEdges, "%d,%d,Directed,%d,%d,%f\n",
            (qtdPointsI>qtdPointsJ?j:i),
            (qtdPointsI>qtdPointsJ?i:j),
            labelEdge,labelEdge,
            (gravitational?
             (qtdPointsI * qtdPointsJ * 6.67e-11) /
             pow(listCells[i]->getCenterMass().dist(listCells[j]->getCenterMass()),2):
             listCells[i]->getCenterMass().dist(listCells[j]->getCenterMass()))
            );

        labelEdge++;

      }
    }
  }

  g.connectedComponents(); // Verify the connected components of graph

  // Create a file to be used on Decision Tree generator
  pFileDT = fopen(outputDT.c_str(), "w");

  if (pFileDT == NULL)
  {
    cout << "Error: Unable do open decision tree output file." << endl;
  }
  else
  {
    // Create header

    // Labels from coordinates
    for (int i=0; i < dimension; i++)
      fprintf(pFileDT, "Coord-%d,", i);

    fprintf(pFileDT, "class\n");

    for (int i = 0; i < listCells.size(); i++)
    {
      vector <vector <double> > vCube = listCells[i]->cubeCoord(epsilon);
      for (int j = 0; j < vCube.size(); j++)
      {
        for (int k = 0; k < vCube[j].size(); k++)
        {
          fprintf(pFileDT, "%f,", vCube[j][k]);
        }
        fprintf(pFileDT, "%s\n", (listCells[i]->getQtyPoints()>=minPoints?"yes":"no"));
      }
    }
    fclose(pFileDT);
  }


  // If dimension == 2, create a svg file
  if (dimension == 2)
  {
    float graphMult = 50.0;

    svgFile.open(outputSVG, std::ifstream::out);
    svgFile << "<?xml version=\"1.0\" standalone=\"no\"?>" << endl
      << "<svg width=\"" << graphMult*1.2 << "cm\" height=\""
      << graphMult * 1.2 << "cm\" version=\"1.1\"" << endl
      << "xmlns=\"http://www.w3.org/2000/svg\">" << endl
      << "<desc>Graph bi-dimensional </desc>" << endl
      // Display External Rectangle
      << "<rect x=\"" << graphMult * 0.1 << "cm\" "
      << "y=\"" << graphMult * 0.1 << "cm\" "
      << "width=\"" << graphMult << "cm\" "
      << "height=\"" << graphMult << "cm\" "
      << "fill=\"none\" stroke=\"black\" stroke-width=\""
      << graphMult * 0.002 << "cm\" />" << endl;
    // Display Text with parameters
    svgFile << "<text x=\"" << graphMult * 0.1 << "cm\" "
      << "y=\"" << graphMult * 0.07 << "cm\" "
      << "font-family=\"Times New Roman\" font-size=\""
      << graphMult * 0.03 << "cm\" fill=\"black\">"
      << "File Name: " << input << ", Epsilon: " << epsilon
      << ", Min Points: " << minPoints
      << "</text>" << endl;

    // Draw Horizontal and Vertical Grids
    svgFile << "<g stroke=\"gray\" stroke-width=\""
      << graphMult * 0.025 / epsilon << "cm\">" << endl;
    for (int i = 1; i < epsilon; i++)
    {
      // Vertical Grids
      svgFile << "<line x1=\""
        << (graphMult * 1/(float)epsilon * i) + graphMult*0.1<< "cm\" "
        << "y1=\"" << graphMult * .1 << "cm\" "
        << "x2=\"" << (graphMult * 1/(float)epsilon * i) + graphMult*0.1<< "cm\" "
        << "y2=\"" << graphMult * 1.1 << "cm\" />" << endl;
      // Horizontal Grids
      svgFile << "<line y1=\""
        << (graphMult * 1/(float)epsilon * i) + graphMult*0.1<< "cm\" "
        << "x1=\"" << graphMult * .1 << "cm\" "
        << "y2=\"" << (graphMult * 1/(float)epsilon * i) + graphMult*0.1<< "cm\" "
        << "x2=\"" << graphMult * 1.1 << "cm\" />" << endl;
    }
    svgFile <<"</g>" << endl;

    // Draw points

    if (drawPoints)
    {
      for (int i = 0; i < pointsSample.size(); i++)
      {
        Point sample = pointsSample[i];
        svgFile << "<circle fill=\"red\" cx=\""
          << (float)(sample.coord[0]*graphMult)+graphMult*.1 << "cm\" "
          << "cy=\"" << (float) graphMult*1.1-(float)(sample.coord[1]*graphMult)
          << "cm\" r=\"" << graphMult * 0.002 << "cm\""
          << " />" << endl;
      }
    }

    // Draw Edges
    vector<string> colors;
    colors.push_back("#007F7F");
    colors.push_back("#7F7F00");
    colors.push_back("#7F007F");
    colors.push_back("#FF7F00");
    colors.push_back("#FF007F");
    colors.push_back("#00007F");
    colors.push_back("#0000FF");
    colors.push_back("#7F0000");
    colors.push_back("#007F00");

    svgFile << "<g stroke=\"blue\" stroke-width=\"" <<
      graphMult * 0.05 / epsilon <<
      "cm\">" << endl;

    for (int i = 0; i<listCells.size(); i++)
    {
      if (listCells[i]->getQtyPoints() < minPoints)
        continue;

      // Draw transparent rect

      if (drawRects)
      {
        string opacity;
        opacity = (!drawPoints||!drawEdges?"0.4":"0.2");
        svgFile << "<rect style=\"opacity:"<<opacity<<";fill:"
          << colors[g.getClassLabel(i)%colors.size()]
          << ";stroke:none\" "
          << "width=\"" << graphMult * 1/(float)epsilon << "cm\" "
          << "height=\"" << graphMult * 1/(float)epsilon << "cm\" "
          << "x=\"" << graphMult * (double)listCells[i]->coord[0]/epsilon + graphMult*.1
          << "cm\" "
          << "y=\"" << graphMult*1.1 - graphMult *
          ((double)listCells[i]->coord[1]/epsilon + (1./epsilon)) << "cm\" />"
          << endl;;
      }

      for (int j = i+1; j<listCells.size() && drawEdges; j++)
      {
        // Verify the quantity of points
        if (listCells[j]->getQtyPoints() < minPoints)
          continue;

        if (areAdjacents(listCells[i], listCells[j]))
        {
          /*
             svgFile << "<text x=\""
             << (float)(listCells[i]->getCenterMass().coord[0]*graphMult)+graphMult*.1
             <<"cm\" " << "y=\""
             << graphMult*1.1 -
             (float)(listCells[i]->getCenterMass().coord[1]*graphMult)
             << "cm\" "
             << "font-family=\"Times New Roman\" font-size=\""
             << graphMult * 0.03 << "cm\" fill=\"black\"> "
             << g.getClassLabel(i) << " "
             << "</text>" << endl;
             */

          svgFile << "<line stroke=\"";
          svgFile << colors[g.getClassLabel(i)%colors.size()] << "\" ";
          svgFile << "x1=\""
            << (float)(listCells[i]->getCenterMass().coord[0]*graphMult)+graphMult*.1
            <<"cm\" " << "y1=\""
            << graphMult*1.1 -
            (float)(listCells[i]->getCenterMass().coord[1]*graphMult)
            << "cm\" " << "x2=\""
            << (float)(listCells[j]->getCenterMass().coord[0]*graphMult)+graphMult*.1
            << "cm\" " << "y2=\""
            << graphMult*1.1 -
            (float)(listCells[j]->getCenterMass().coord[1]*graphMult)
            << "cm\" />" << endl;
        }

      }
    }

    if (dimension == 2)
      svgFile << "</g>" << endl;

  }

  svgFile << "</svg>";
  svgFile.close();

  cout << "------------------" << endl;
  cout << "Qty Cells with more then " << minPoints << " points: " << qtyCellsVal << endl;

  infile.close();
  fclose(pFileNodes);
  fclose(pFileEdges);
}
