class Point:
	self.x
	self.y
	
	def __init__(self, _x, _y):
		self.x = _x
		self.y = _y
		
	def __str__(self):
		return '\n(%u %u)' % (self.x, self.y)
		
class PointVector:
	self.vec = []
	
	def new_point(self):
		self.vec.append(Point())
        
    def __str__(self):
        s = ''
        for p in self.vec:
            s += p.__str__()
   
    
def do_first(vector):
    for point in vector:
        self.x = self.x - 5
        self.y = self.y - 1
    
def do_second(vector)    
       for point in vector:
        self.x = self.x - 8
        self.y = self.y - 4
    
    
def fill_vector():
    nums = [[2, -10], [5, -9], [4, -7],
            [7, -10], [10, -8], [6, -5], 
            [10, -2], [7, 0], [4, -5]
            [3, 0], [0, -2], [2, -10]]
            
    vector = PointVector()        
            
    for p in nums:
        vector.new_point(p[0], p[1])
    
    return vector

def main():
    vector = fill_vector()
    print(vector)
    do_first(vector)
    print(vector)
    do_second()
    print(vector)    
   
        