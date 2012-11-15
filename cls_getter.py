f = open('teva_llawan.txt', 'r')
g = open('teva_llawan_condensed.txt', 'w')
for line in f:
   if (line.find('P values for  sigma = ') != -1 or line.find('CL') != -1 or line.find('Test Statistic on data') != -1):
        g.write(line)
        g.write('\n')
