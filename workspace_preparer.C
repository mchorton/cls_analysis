/*
 * Max Horton
 * CERN Summer Student 2012
 * Caltech Class of 2014
 * maxwell.christian.horton@gmail.com
 * 
 * This code sets up a workspace with pdfs and variables / expressions.
 * The workspace is used in a call to the RooStats StandardHypoTestInv.C,
 * to calculate CLs for inputs.  The program works in conjunction with a
 * config file (this code comes with an example).
 *
 * The config file contains declarations of variable values.  This file
 * creates these variables from the config file using a config_reader.
 * 
 * The model:
 * Parameter of Interest: sigma (cross section)
 * Nuisance Parameters: luminosity, efficiency, rho (transfer factor)
 * 
 * Specifications: This program requires the input .root files.  One with
 * a signal model TH2D, one with a background-only sample TH2D, and one
 * with a data model TH2D.  It creates pdfs from the signal and
 * background-only TH2Ds, then extends them by S = lumi * efficiency *
 * cross section, and B = bprime (the integral over the background region)
 * * rho (the transfer factor).  It adds the pdfs, then multiplies them
 * by gaussian penalty terms for the nuisance parameters.  This creates 
 * the overall signal + background model.  Then, the background-only model
 * is created by fixing sigma = 0 for this model (to indicate no cross -
 * section, i.e. 0 signal events).
 *
 * Adapted from roostats_twobin.C, by Fedor Ratnikov and Gena Kukartsev
 */


#include "TStopwatch.h"
#include "TCanvas.h"
#include "TROOT.h"

#include "RooPlot.h"
#include "RooAbsPdf.h"
#include "RooWorkspace.h"
#include "RooDataSet.h"
#include "RooGlobalFunc.h"
#include "RooFitResult.h"
#include "RooRandom.h"
#include "RooArgSet.h"

#include "RooStats/RooStatsUtils.h"
#include "RooStats/ProfileLikelihoodCalculator.h"
#include "RooStats/LikelihoodInterval.h"
#include "RooStats/LikelihoodIntervalPlot.h"
#include "RooStats/BayesianCalculator.h"
#include "RooStats/MCMCCalculator.h"
#include "RooStats/MCMCInterval.h"
#include "RooStats/MCMCIntervalPlot.h"
#include "RooStats/ProposalHelper.h"
#include "RooStats/SimpleInterval.h"
#include "RooStats/FeldmanCousins.h"
#include "RooStats/PointSetInterval.h"

using namespace RooFit;
using namespace RooStats;


// Functions
void workspace_preparer(char *signal_file_name = "signal.root", 
                        char *signal_hist_name_in_file = "signal", 
                        char *background_file_name = "background.root",
                        char *background_hist_name_in_file = "background",
                        char *data_file_name = "data.root", 
                        char *data_hist_name_in_file = "data", 
                        char *config_file = "config_unibin",
                        char *workspace_name = "ws_twobin.root");
void SetConstants(RooWorkspace * w, RooStats::ModelConfig * mc);
void SetConstant(const RooArgSet * vars, Bool_t value );

/*
 * Prepares the workspace to be used by the hypothesis test calculator
 */
void workspace_preparer(char *signal_file_name, char *signal_hist_name_in_file, char *background_file_name, char *background_hist_name_in_file, char *data_file_name, char *data_hist_name_in_file, char *config_file, char *workspace_name){

  // Include the config_reader class.
  TString path = gSystem->GetIncludePath();
  path.Append(" -I/home/max/cern/cls/mario");//why does this work?
  gSystem->SetIncludePath(path);
  gROOT->LoadMacro("config_reader.cxx");

  // RooWorkspace used to store values.
  RooWorkspace * pWs = new RooWorkspace("ws");

  // Create a config_reader (see source for details) to read the config
  // file.
  config_reader reader(config_file, pWs);

  // Read MR and RR bounds from the config file.
  double MR_lower = reader.find_double("MR_lower");
  double MR_upper = reader.find_double("MR_upper");
  double RR_lower = reader.find_double("RR_lower");
  double RR_upper = reader.find_double("RR_upper");
  double MR_initial = (MR_lower + MR_upper)/2;
  double RR_initial = (RR_lower + RR_upper)/2;

  // Define the Razor Variables 
  RooRealVar MR = RooRealVar("MR", "MR", MR_initial, MR_lower, MR_upper);
  RooRealVar RR = RooRealVar("RSQ", "RSQ", RR_initial, RR_lower, RR_upper);

  // Argument lists 
  RooArgList pdf_arg_list(MR, RR, "input_args_list");
  RooArgSet pdf_arg_set(MR, RR, "input_pdf_args_set");



  /***********************************************************************/
  /* PART 1: IMPORTING SIGNAL AND BACKGROUND HISTOGRAMS                  */
  /***********************************************************************/

  /*
   * Get the signal's unextended pdf by converting the TH2D in the file
   * into a RooHistPdf
   */
  TFile *signal_file = new TFile(signal_file_name);
  TH2D *signal_hist = (TH2D *)signal_file->Get(signal_hist_name_in_file);
  RooDataHist *signal_RooDataHist = new RooDataHist("signal_roodatahist",
                                                    "signal_roodatahist", 
                                                    pdf_arg_list, 
                                                    signal_hist);

  RooHistPdf *unextended_sig_pdf = new RooHistPdf("unextended_sig_pdf", 
                                                  "unextended_sig_pdf", 
                                                  pdf_arg_set, 
                                                  *signal_RooDataHist);

  /* 
   * Repeat this process for the background.
   */
  TFile *background_file = new TFile(background_file_name);
  TH2D *background_hist = 
    (TH2D *)background_file->Get(background_hist_name_in_file);
  RooDataHist *background_RooDataHist = 
    new RooDataHist("background_roodatahist", "background_roodatahist", 
                    pdf_arg_list, background_hist);
  RooHistPdf *unextended_bkg_pdf = new RooHistPdf("unextended_bkg_pdf", 
                                                  "unextended_bkg_pdf", 
                                                  pdf_arg_set, 
                                                  *background_RooDataHist);

  /*
   * Now, we want to create the bprime variable, which represents the
   * integral over the background-only sample.  We will perform the
   * integral automatically (that's why this is the only nuisance
   * parameter declared in this file - its value can be determined from
   * the input histograms).
   */
  ostringstream bprime_string;

  double integral = background_hist->Integral();
  double low_tolerance = .8 * integral;//1.3 * integral;
  double high_tolerance = 1.2 * integral;//.7 * integral;

  double nom_val = integral + 1;
  double low_nom = ((low_tolerance / integral) * nom_val);
  double high_nom = ((high_tolerance / integral) * nom_val);

  // Make a poisson in prime_bprime and nom_bprime
  //bprime_string << "Poisson::bprime_pdf(nom_bprime[" <<  integral << "," << (integral - 1) << "," << (integral + 1) << "], prime_bprime[" << integral << "," << (low_tolerance)  << "," << (high_tolerance)  << "])";
  bprime_string << "Gamma::bprime_pdf(prime_bprime[" << integral << "," << low_tolerance << "," << high_tolerance << "], nom_bprime[" << nom_val << "," << low_nom << "," << high_nom << "], 1, 0)";

  cout << endl << endl << endl << endl << bprime_string.str() << endl << endl;

  pWs->factory(bprime_string.str().c_str());


  /* 
   * This simple command will create all values from the config file
   * with 'make:' at the beginning and a delimiter at the end (see config
   * _reader if you don't know what a delimiter is).  In other
   * words, the luminosity, efficiency, transfer factors, and their pdfs
   * are created from this command.  The declarations are contained in the
   * config file to be changed easily without having to modify this code.
   */
  reader.factory_all();


  /* 
   * Now, we want to create the extended pdfs from the unextended pdfs, as
   * well as from the S and B values we manufactured in the config file.
   * S and B are the values by which the signal and background pdfs,
   * respectively, are extended.  Recall that they were put in the
   * workspace in the reader.facotry_all() command.
   */
  RooAbsReal *S = pWs->function("S");
  RooAbsReal *B = pWs->function("B");
  
  RooExtendPdf *signalpart = new RooExtendPdf("signalpart", "signalpart",
                                              *unextended_sig_pdf, *S);
  RooExtendPdf *backgroundpart = 
    new RooExtendPdf("backgroundpart", "backgroundpart", 
                     *unextended_bkg_pdf, *B);

  RooArgList *pdf_list = new RooArgList(*signalpart, *backgroundpart, 
                                        "list");
  // Add the signal and background pdfs to make a TotalPdf
  RooAddPdf *TotalPdf = new RooAddPdf("TotalPdf", "TotalPdf", *pdf_list);
  
  RooArgList *pdf_prod_list = new RooArgList(*TotalPdf, 
                                             *pWs->pdf("lumi_pdf"),
                                             *pWs->pdf("eff_pdf"),
                                             *pWs->pdf("rho_pdf"),
                                             *pWs->pdf("bprime_pdf"));
  // This creates the final model pdf.
  RooProdPdf *model = new RooProdPdf("model", "model", *pdf_prod_list);

  /*
   * Up until now, we have been using the workspace pWs to contain all of
   * our values.  Now, all of our values that we require are in use in the
   * RooProdPdf called "model".  So, we need to import "model" into a 
   * RooWorkspace.  To avoid recopying values into the rooworkspace, when
   * the values may already be present (which can cause problems), we will
   * simply create a new RooWorkspace to avoid confusion and problems.  The
   * new RooWorkspace is created here.
   */
  RooWorkspace *newworkspace = new RooWorkspace("newws");
  newworkspace->import(*model);

  // Immediately delete pWs, so we don't accidentally use it again.
  delete pWs;

  // Show off the newworkspace
  newworkspace->Print();

  // observables
  RooArgSet obs(*newworkspace->var("MR"), *newworkspace->var("RSQ"), "obs");

  // global observables
  //RooArgSet globalObs(*newworkspace->var("nom_lumi"), *newworkspace->var("nom_eff"), *newworkspace->var("nom_rho"), *newworkspace->var("nom_bprime"));
  RooArgSet globalObs(*newworkspace->var("nom_lumi"), *newworkspace->var("nom_eff"), *newworkspace->var("nom_rho"), *newworkspace->var("nom_bprime"), "global_obs");

  //fix global observables to their nominal values
  newworkspace->var("nom_lumi")->setConstant();
  newworkspace->var("nom_eff")->setConstant();
  newworkspace->var("nom_rho")->setConstant();
  newworkspace->var("nom_bprime")->setConstant();//Do this?  probs fine.

  //Set Parameters of interest
  RooArgSet poi(*newworkspace->var("sigma"), "poi");  


  //Set Nuisances

  RooArgSet nuis(*newworkspace->var("prime_lumi"), *newworkspace->var("prime_eff"), *newworkspace->var("prime_rho"), *newworkspace->var("prime_bprime"));

  // priors (for Bayesian calculation)
  //newworkspace->factory("Uniform::prior_signal(sigma)"); // for parameter of interest
  //newworkspace->factory("Uniform::prior_bg_b(prime_bprime)"); // for data driven nuisance parameter
  //newworkspace->factory("PROD::prior(prior_signal,prior_bg_b)"); // total prior


  //Observed data is pulled from histogram.
  /*TFile *data_file = new TFile(data_file_name);
  TH2D *data_hist = (TH2D *)data_file->Get(data_hist_name_in_file);
  RooDataHist *pData = new RooDataHist("data", "data", obs, data_hist);
  newworkspace->import(*pData);*/

  // Now, we will draw our data from a RooDataHist.
  TFile *data_file = new TFile(data_file_name);
  //TTree *data_tree = (TTree *) data_file->Get(data_hist_name_in_file);
  RooDataSet *pData = (RooDataSet *)data_file->Get(data_hist_name_in_file);
  newworkspace->import(*pData);
  

  // Craft the signal+background model
  ModelConfig * pSbModel = new ModelConfig("SbModel");
  pSbModel->SetWorkspace(*newworkspace);
  pSbModel->SetPdf(*newworkspace->pdf("model"));
  //pSbModel->SetPriorPdf(*newworkspace->pdf("prior"));//[][]
  pSbModel->SetParametersOfInterest(poi);
  pSbModel->SetNuisanceParameters(nuis);
  pSbModel->SetObservables(obs);
  pSbModel->SetGlobalObservables(globalObs);

  // set all but obs, poi and nuisance to const
  SetConstants(newworkspace, pSbModel);
  newworkspace->import(*pSbModel);


  // background-only model
  // use the same PDF as s+b, with sig=0
  // POI value under the background hypothesis
  // (We will set the value to 0 later)

  Double_t poiValueForBModel = 0.0;
  ModelConfig* pBModel = new ModelConfig(*(RooStats::ModelConfig *)newworkspace->obj("SbModel"));
  pBModel->SetName("BModel");
  pBModel->SetWorkspace(*newworkspace);
  newworkspace->import(*pBModel);

  // find global maximum with the signal+background model
  // with conditional MLEs for nuisance parameters
  // and save the parameter point snapshot in the Workspace
  //  - safer to keep a default name because some RooStats calculators
  //    will anticipate it
  RooAbsReal * pNll = pSbModel->GetPdf()->createNLL(*pData);
  RooAbsReal * pProfile = pNll->createProfile(RooArgSet());
  pProfile->getVal(); // this will do fit and set POI and nuisance parameters to fitted values
  RooArgSet * pPoiAndNuisance = new RooArgSet();
  if(pSbModel->GetNuisanceParameters())
    pPoiAndNuisance->add(*pSbModel->GetNuisanceParameters());
  pPoiAndNuisance->add(*pSbModel->GetParametersOfInterest());
  cout << "\nWill save these parameter points that correspond to the fit to data" << endl;
  pPoiAndNuisance->Print("v");
  pSbModel->SetSnapshot(*pPoiAndNuisance);
  delete pProfile;
  delete pNll;
  delete pPoiAndNuisance;


  // Find a parameter point for generating pseudo-data
  // with the background-only data.
  // Save the parameter point snapshot in the Workspace
  pNll = pBModel->GetPdf()->createNLL(*pData);
  pProfile = pNll->createProfile(poi);
  ((RooRealVar *)poi.first())->setVal(poiValueForBModel);
  pProfile->getVal(); // this will do fit and set nuisance parameters to profiled values
  pPoiAndNuisance = new RooArgSet();
  if(pBModel->GetNuisanceParameters())
    pPoiAndNuisance->add(*pBModel->GetNuisanceParameters());
  pPoiAndNuisance->add(*pBModel->GetParametersOfInterest());
  cout << "\nShould use these parameter points to generate pseudo data for bkg only" << endl;
  pPoiAndNuisance->Print("v");
  pBModel->SetSnapshot(*pPoiAndNuisance);
  delete pProfile;
  delete pNll;
  delete pPoiAndNuisance;

  // save workspace to file
  newworkspace->writeToFile(workspace_name);

  // clean up
  delete newworkspace;
  delete pData;
  delete pSbModel;
  delete pBModel;


} // ----- end of tutorial ----------------------------------------



// helper functions

void SetConstants(RooWorkspace * pWs, RooStats::ModelConfig * pMc){
  //
  // Fix all variables in the PDF except observables, POI and
  // nuisance parameters. Note that global observables are fixed.
  // If you need global observables floated, you have to set them
  // to float separately.
  //

  pMc->SetWorkspace(*pWs);

  RooAbsPdf * pPdf = pMc->GetPdf(); // we do not own this

  RooArgSet * pVars = pPdf->getVariables(); // we do own this

  RooArgSet * pFloated = new RooArgSet(*pMc->GetObservables());
  pFloated->add(*pMc->GetParametersOfInterest());
  pFloated->add(*pMc->GetNuisanceParameters());

  TIterator * pIter = pVars->createIterator(); // we do own this

  for(TObject * pObj = pIter->Next(); pObj; pObj = pIter->Next() ){
    std::string _name = pObj->GetName();
    RooRealVar * pFloatedObj = (RooRealVar *)pFloated->find(_name.c_str());
    if (pFloatedObj){
      ((RooRealVar *)pObj)->setConstant(kFALSE);
    }
    else{
      ((RooRealVar *)pObj)->setConstant(kTRUE);
    }
  }

  delete pIter;
  delete pVars;
  delete pFloated;

  return;
}



void SetConstant(const RooArgSet * vars, Bool_t value ){
  //
  // Set the constant attribute for all vars in the set
  //

  TIterator * pIter = vars->createIterator(); // we do own this

  for(TObject * pObj = pIter->Next(); pObj; pObj = pIter->Next() ){
    ((RooRealVar *)pObj)->setConstant(value);
  }

  delete pIter;

  return;
}


