# Max Horton
# CERN summer student 2012
# Caltech Class of 2014
# Maxwell.Christian.Horton@gmail.com
#
# Entry point for the CLs calculator.  Note that the CLs calculator in
# this case is the StandardHypoTestInvDemo() program, from RooStats.  It
# will frequently be referred to as simply "the calculator", because
# that's what it is.
#
# The program is called from the command line using
# "python cls_analysis.py" with optional flags.  The CLs calculator will
# run various styles of CLs calulation on inputs.  We require three data
# .root files - one with a signal model, one with a background-rich
# region, and the data.  Their names can be specified as flags (see
# below).  The models should be stored in the .root files as TH2D objects.
#
# Additionally, we require a config file with information about the
# physical model.  It contains information such as the luminosity,
# efficiency, etc.  This information was outsourced to a config file
# rather than put directly in the code so that the code doesn't have
# to be modified to test it on a different set of data with different
# parameters.  The code allows for the specification of config file names,
# so one can easily run the code on a variety of different inputs just
# by changing the config file name and not the code.
#
# The model: We have sigma (the cross section) as our parameter of
# interest.  We want to perform a CLs test using this parameter.  The
# StandardHypoTestInvDemo() will perform calculations for us, but I need
# to set up a workspace for it to read values from.

# Histograms are constructred from the provided TH2Ds.
# Variables for the nuisance parameters (luminosity, efficiency,
# transfer factor, and control region count) are created.  They are
# modeled by lognormals.  The signal and background unextended pdfs
# are created from the signal model TH2D and the control region TH2D.
# Extended pdfs are made by extending the signal and background pdfs by
# S = sigma * luminosity * efficiency, and B = bprime * rho, respectively,
# Where sigma is the parameter of interest (cross section), bprime is the
# count of the data in the background-only sample (the TH2D's Integral()
# value), and rho is the transfer factor.  The extended pdfs are added,
# then multiplied by the gaussian penalty terms for the luminosity, 
# efficiency, and transfer factor, and a poisson penalty term for 
# the background count.  See the config file and the workspace_preparer 
# files for details of how the model is computed.

# By default, a one-tailed test is used.  That's why testStat is set to 3.
# See StandardHypoTestInvDemo.C for how to change options.


from ROOT import *
import os
from optparse import OptionParser

# Set up an OptionParser with desired options, and return it.
def define_parser():
    # Note that we are using the options as values in calls to the system.
    # So we might as well just store them as strings (thus all of them are
    # strings.
    parser = OptionParser()
    parser.add_option("-c", "--config_file_name", action="store", type="string", dest="config_file_name")
    parser.add_option("-a", "--calculatorType", action="store", type="string", dest="calculatorType")
    parser.add_option("-t", "--test_statistic_type", action="store", type="string", dest="testStatType")
    parser.add_option("-p", "--points", action="store", type="string", dest="npoints")
    parser.add_option("-n", "--num_toys", action="store", type="string", dest="ntoys")
    parser.add_option("--sfile", "--signal_histogram_file", action="store", type = "string", dest = "input_sig")
    parser.add_option("--bfile", "--background_histogram_file", action="store", type = "string", dest = "input_bkg")
    parser.add_option("--dfile", "--data_histogram_file", action="store", type = "string", dest = "input_dat")
    parser.add_option("--signame", action="store", type = "string", dest = "signame")
    parser.add_option("--bkgname", action="store", type = "string", dest = "bkgname")
    parser.add_option("--datname", action="store", type = "string", dest = "datname")
    parser.add_option("-b", action="store_true", dest = "suppress")
    return parser


# Main function.  It will read the input options, creating values for
# strings to correspond to the user's requested options.  Then, the
# workspace_preparer will be called, to prepare the model for the 
# calculation.  Then, the actual CLs calculator will be called.
if __name__ == "__main__":
    parser = define_parser()
    (options, args) = parser.parse_args()

    # Set defaults for optional values
    # Note that some of these aren't available as flags, because they
    # aren't likely to be changed by the user.  (Ex: useCLs should always
    # be set to true because we are doing CLs!)
    config_file_name = "config.cfg"
    # These \\" ... \\" are needed because we perform an os.system call
    # to run the calculator (see comment below explaining why).  When
    # a system call is performed in this way, ROOT requires that all
    # string arguments to the call be surrounded by \".  So, we need
    # \\" on either side (since printing a backslash requires the 
    # backslash escape character.
    #
    # See StandardHypoTestInvDemo() for explanation of parameters such
    # as calculatorType or testStatType
    wsName = '\\"newws\\"'
    modelSBName = '\\"SbModel\\"'
    modelBName = '\\"BModel\\"'
    dataName = '\\"data\\"'
    calculatorType = "0" 
    testStatType = "3" 
    useCLs = "true"
    npoints = "4"
    poimin = "0"
    poimax = "1000"
    ntoys = "1000"
    useNumberCounting = "false"
    nuisPriorName = '\\"0\\"'

    # When we call the workspace_preparer, we aren't doing an os.system()
    # call, so we don't need the \\" characters.
    input_sig = "signal.root"
    input_bkg = "background.root"
    input_dat = "data.root"
    signame = "signal"
    bkgname = "background"
    datname = "data"

    graphics_string = ''

    # Set up all values according to the input parser
    if (options.config_file_name != None):
        config_file_name = options.config_file_name

    if (options.calculatorType != None):
        calculatorType = options.calculatorType

    if (options.testStatType != None):
        testStatType = options.testStatType

    if (options.npoints != None):
        npoints = options.npoints

    if (options.ntoys != None):
        ntoys = options.ntoys

    if (options.input_sig != None):
        input_sig = options.input_sig

    if (options.input_bkg != None):
        input_bkg = options.input_bkg

    if (options.input_dat != None):
        input_dat = options.input_dat

    if (options.signame != None):
        signame = options.signame

    if (options.bkgname != None):
        bkgname = options.bkgname

    if (options.datname != None):
        datname = options.datname

    if (options.suppress):
        graphics_string = '-b -q '


    workspace = "_workspacefrom_" + input_sig + input_bkg + input_dat
    infile = '\\"' + workspace + '\\"'



    cls_name = '\\"' + input_dat + '_cls.ps\\"'
    bells_name = '\\"' + input_dat + '_bells.ps\\"'
    # Unfortunately, the StandardHypoTestInvDemo() method, which is used
    # to perform the calculations, has an issue: if we try to load it
    # running the workspace_preparer, it won't load all of its packages.
    # So, we have to resort to an os.system() call to get the function to
    # work.  Thus, we need to create a large, ROOT-formatted string to
    # contain the information we need to pass in the system call.  This is
    # the hypotest_call_string below.
    hypotest_call_string =  infile + ',' + wsName + ',' + modelSBName + ',' + modelBName + ',' + dataName + ',' + calculatorType + ',' + testStatType + ',' + useCLs + ',' + npoints + ',' + poimin + ',' + poimax + ',' + ntoys + ',' + useNumberCounting + ',' + nuisPriorName + ','  + cls_name + "," + bells_name

    # Now that we have set up for the calls, let's make them.  First, load
    # the workspace_preparer, which sets up a file for the StandardHypo-
    # TestInvDemo() to read values / pdfs from.
    gROOT.ProcessLine('.L workspace_preparer.C')

    # Now we can prepare the workspace.
    workspace_preparer(input_sig, signame, input_bkg, bkgname, input_dat, datname, config_file_name, workspace)

    # As mentioned, gROOT.ProcessLine('.L StandardHypoTestInvDemo.C')
    # and then StandardHypoTestInvDemo() will cause errors because the 
    # packages needed won't load.  So, we resort to a different method of
    # calling the function - a simple os.system call.
    os.system('root -l ' + graphics_string  + ' "StandardHypoTestInvDemo.C(' + hypotest_call_string  + ')" -q')
